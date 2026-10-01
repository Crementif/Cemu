#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanRenderer.h"
#include "Cafe/HW/Latte/Renderer/Vulkan/LatteTextureVk.h"
#include "Cafe/HW/Latte/Renderer/Vulkan/LatteTextureViewVk.h"
#include "Cafe/HW/Latte/Renderer/Vulkan/CachedFBOVk.h"
#include "Cafe/HW/Latte/Core/LatteShader.h"
#include "Cafe/HW/Latte/Core/FetchShader.h"
#include "Cafe/HW/Latte/Core/LatteView.h"
#include "Cafe/HW/Latte/ISA/RegDefines.h"

// in a stereo scope the draws get recorded and replayed for the right view once the left pass ends

float* VulkanRenderer_GetUniformData(uint32 shaderStageIndex);

void VulkanRenderer::view_setStereo(bool isStereo)
{
	draw_endRenderPass();
	m_view.isStereo = isStereo;
}

LatteTextureView* VulkanRenderer::view_getSiblingView(LatteTextureView* view, bool forAttachment)
{
	LatteTexture* texture = view->baseTexture;
	LatteTexture* sibling = forAttachment ? LatteView_GetOrCreateSibling(texture) : texture->viewSibling;
	if (!sibling)
		return nullptr;
	return sibling->GetOrCreateView(view->dim, view->format, view->firstMip, view->numMip, view->firstSlice, view->numSlice);
}

CachedFBOVk* VulkanRenderer::view_getRightFBO(CachedFBOVk* leftFBO)
{
	CachedFBOVk* rightFBO = nullptr;
	auto it = m_view.rightFBOs.find(leftFBO);
	if (it != m_view.rightFBOs.end())
		rightFBO = it->second;
	if (!rightFBO)
	{
		LatteTextureView* colorViews[8]{};
		for (sint32 i = 0; i < 8; i++)
		{
			if (!leftFBO->colorBuffer[i].texture)
				continue;
			colorViews[i] = view_getSiblingView(leftFBO->colorBuffer[i].texture, true);
			if (!colorViews[i])
				return nullptr;
		}
		LatteTextureView* depthView = nullptr;
		if (leftFBO->depthBuffer.texture)
		{
			depthView = view_getSiblingView(leftFBO->depthBuffer.texture, true);
			if (!depthView)
				return nullptr;
		}
		rightFBO = (CachedFBOVk*)LatteMRT::CreateDetachedCachedFBO(colorViews, depthView, leftFBO->depthBuffer.hasStencil);
		m_view.rightFBOs[leftFBO] = rightFBO;
	}
	// the left pass might not cover the whole target
	for (sint32 i = 0; i < 8; i++)
	{
		LatteTextureView* view = leftFBO->colorBuffer[i].texture;
		if (view && !LatteView_IsSiblingValid(view->baseTexture, view->firstSlice, view->firstMip))
			LatteView_SeedSibling(view->baseTexture, view->firstSlice, view->firstMip);
	}
	if (LatteTextureView* view = leftFBO->depthBuffer.texture)
	{
		if (!LatteView_IsSiblingValid(view->baseTexture, view->firstSlice, view->firstMip))
			LatteView_SeedSibling(view->baseTexture, view->firstSlice, view->firstMip);
	}
	return rightFBO;
}

void VulkanRenderer::view_notifyFBODeleted(LatteCachedFBO* fbo)
{
	if (m_view.rightFBOs.empty())
		return;
	if (fbo == m_view.rightFBO)
		m_view.rightFBO = nullptr;
	auto it = m_view.rightFBOs.find((CachedFBOVk*)fbo);
	if (it != m_view.rightFBOs.end())
	{
		CachedFBOVk* rightFBO = it->second;
		m_view.rightFBOs.erase(it);
		LatteMRT::DeleteCachedFBO(rightFBO);
		return;
	}
	std::erase_if(m_view.rightFBOs, [fbo](const auto& entry) { return entry.second == fbo; });
}

void VulkanRenderer::view_notifySiblingDeleted(LatteTexture* texture)
{
	std::erase_if(m_view.pendingSeeds, [texture](const auto& seed) { return seed.first == texture; });
}

void VulkanRenderer::view_notifyDescriptorSetDeleted(VkDescriptorSetInfo* dsInfo)
{
	if (m_view.rightDescriptorSets.empty())
		return;
	if (m_view.rightDescriptorSets.erase(dsInfo) == 0 && m_view.knownRightDescriptorSets.count(dsInfo) == 0)
		return;
	if (m_view.knownRightDescriptorSets.erase(dsInfo) != 0)
		std::erase_if(m_view.rightDescriptorSets, [dsInfo](const auto& entry) { return entry.second.set == dsInfo; });
}

void VulkanRenderer::view_prepareRightPass(CachedFBOVk* leftFBO)
{
	m_view.isRecording = false;
	if (!m_view.isStereo)
		return;
	CachedFBOVk* rightFBO = view_getRightFBO(leftFBO);
	if (!rightFBO)
	{
		cemuLog_logOnce(LogType::Force, "LatteView: A render target has no right-view sibling, the right view skips its draws");
		return;
	}
	m_view.rightFBO = rightFBO;
	m_view.renderPass = rightFBO->GetRenderPassObj()->m_renderPass;
	m_view.framebuffer = rightFBO->GetFramebufferObj()->m_frameBuffer;
	m_view.extent = rightFBO->GetExtend();
	rightFBO->GetRenderPassObj()->flagForCurrentCommandBuffer();
	rightFBO->GetFramebufferObj()->flagForCurrentCommandBuffer();
	for (auto& tex : rightFBO->GetTextures())
		((LatteTextureVk*)tex)->GetImageObj()->flagForCurrentCommandBuffer();
	if (m_featureControl.deviceExtensions.dynamic_rendering)
	{
		// the FBO can be deleted before the replay
		const VkRenderingInfoKHR* renderingInfo = rightFBO->GetRenderingInfo();
		m_view.renderingInfo = *renderingInfo;
		cemu_assert_debug(renderingInfo->colorAttachmentCount <= 8);
		for (uint32 i = 0; i < renderingInfo->colorAttachmentCount; i++)
			m_view.colorAttachments[i] = renderingInfo->pColorAttachments[i];
		m_view.renderingInfo.pColorAttachments = m_view.colorAttachments;
		if (renderingInfo->pDepthAttachment)
		{
			m_view.depthAttachment = *renderingInfo->pDepthAttachment;
			m_view.renderingInfo.pDepthAttachment = &m_view.depthAttachment;
		}
		if (renderingInfo->pStencilAttachment)
		{
			m_view.stencilAttachment = *renderingInfo->pStencilAttachment;
			m_view.renderingInfo.pStencilAttachment = &m_view.stencilAttachment;
		}
	}
	m_view.commands.clear();
	m_view.dynamicOffsets.clear();
	m_view.bound = {};
	m_view.bound.depthBias[0] = std::numeric_limits<float>::quiet_NaN();
	m_view.bound.blendConstants[0] = std::numeric_limits<float>::quiet_NaN();
	m_view.bound.feedbackLoopAspect = 0xFFFFFFFF;
	m_view.isRecording = true;
}

bool VulkanRenderer::uniformData_buildRightView(LatteDecompilerShader* shader, const float* leftUniformData, float* rightUniformData)
{
	if (shader->resourceMapping.uniformVarsBufferBindingPoint < 0)
		return false;
	uint32 aluConstBase = 0;
	uint32 uniformBlockRegBase = mmSQ_PS_UNIFORM_BLOCK_START;
	if (shader->shaderType == LatteConst::ShaderType::Vertex)
	{
		aluConstBase = 0x400;
		uniformBlockRegBase = mmSQ_VTX_UNIFORM_BLOCK_START;
	}
	else if (shader->shaderType == LatteConst::ShaderType::Geometry)
		uniformBlockRegBase = mmSQ_GS_UNIFORM_BLOCK_START;
	bool hasRightAluConst = shader->shaderType != LatteConst::ShaderType::Geometry && LatteView_HasRightAluConst(shader->shaderType == LatteConst::ShaderType::Vertex);
	const uint32* rightAluConst = LatteView_GetRightAluConst();

	bool isModified = false;
	auto beginModify = [&]() {
		if (!isModified)
			memcpy(rightUniformData, leftUniformData, shader->uniform.uniformRangeSize);
		isModified = true;
	};
	if (shader->uniform.loc_remapped >= 0)
	{
		uint8* remappedData = (uint8*)rightUniformData + shader->uniform.loc_remapped;
		if (hasRightAluConst)
		{
			for (auto& it : shader->list_remappedUniformEntries_register)
			{
				uint32 registerIndex = aluConstBase + it.indexOffset / 4;
				if (!LatteView_IsRightAluConstVec4(registerIndex / 4))
					continue;
				beginModify();
				memcpy(remappedData + it.mappedIndexOffset, rightAluConst + registerIndex, 16);
			}
		}
		if (LatteView_HasRightWords())
		{
			for (auto& bufferGroup : shader->list_remappedUniformEntries_bufferGroups)
			{
				MPTR physicalAddr = LatteGPUState.contextRegister[uniformBlockRegBase + bufferGroup.kcacheBankIdOffset / 4];
				uint32 size = LatteGPUState.contextRegister[uniformBlockRegBase + bufferGroup.kcacheBankIdOffset / 4 + 1] + 1;
				const uint8* rightBlock = LatteView_GetRightUniformBlock(physicalAddr, size);
				if (!rightBlock)
					continue;
				beginModify();
				for (auto& it : bufferGroup.entries)
				{
					if (it.indexOffset + 16 <= size)
						memcpy(remappedData + it.mappedIndexOffset, rightBlock + it.indexOffset, 16);
				}
			}
		}
	}
	if (shader->uniform.loc_uniformRegister >= 0 && hasRightAluConst)
	{
		uint8* registerData = (uint8*)rightUniformData + shader->uniform.loc_uniformRegister;
		for (sint32 i = 0; i < shader->uniform.count_uniformRegister; i++)
		{
			uint32 vec4Index = aluConstBase / 4 + i;
			if (!LatteView_IsRightAluConstVec4(vec4Index))
				continue;
			beginModify();
			memcpy(registerData + i * 16, rightAluConst + vec4Index * 4, 16);
		}
	}
	return isModified;
}

VkDescriptorSetInfo* VulkanRenderer::view_getRightDescriptorSet(PipelineInfo* pipeline_info, LatteDecompilerShader* shader, VkDescriptorSetInfo* leftDS)
{
	if (!leftDS || !shader)
		return leftDS;
	LatteTextureViewVk** boundTextures = m_state.boundTexture;
	switch (shader->shaderType)
	{
	case LatteConst::ShaderType::Vertex:
		boundTextures += LATTE_CEMU_VS_TEX_UNIT_BASE;
		break;
	case LatteConst::ShaderType::Pixel:
		boundTextures += LATTE_CEMU_PS_TEX_UNIT_BASE;
		break;
	case LatteConst::ShaderType::Geometry:
		boundTextures += LATTE_CEMU_GS_TEX_UNIT_BASE;
		break;
	default:
		UNREACHABLE;
	}
	const sint32 textureCount = shader->resourceMapping.getTextureCount();
	bool usesSibling = false;
	for (sint32 i = 0; i < textureCount && !usesSibling; i++)
	{
		LatteTextureViewVk* view = boundTextures[shader->resourceMapping.getRelativeTextureUnitFromRelativeBindingPoint(i)];
		usesSibling = view && LatteView_HasValidSiblingInRange(view->baseTexture, view->firstSlice, view->numSlice, view->firstMip, view->numMip);
	}
	if (!usesSibling)
		return leftDS;
	uint32 siblingEpoch = LatteView_GetSiblingEpoch();
	auto it = m_view.rightDescriptorSets.find(leftDS);
	if (it != m_view.rightDescriptorSets.end() && it->second.siblingEpoch == siblingEpoch)
		return it->second.set;

	LatteTextureViewVk* leftViews[LATTE_NUM_MAX_TEX_UNITS]{};
	for (sint32 i = 0; i < textureCount; i++)
	{
		sint32 unit = shader->resourceMapping.getRelativeTextureUnitFromRelativeBindingPoint(i);
		LatteTextureViewVk* view = boundTextures[unit];
		leftViews[unit] = view;
		if (!view || !LatteView_HasValidSiblingInRange(view->baseTexture, view->firstSlice, view->numSlice, view->firstMip, view->numMip))
			continue;
		LatteTexture* texture = view->baseTexture;
		for (sint32 mip = view->firstMip; mip < std::min(view->firstMip + view->numMip, texture->mipLevels); mip++)
		{
			for (sint32 slice = view->firstSlice; slice < std::min(view->firstSlice + view->numSlice, texture->depth); slice++)
			{
				if (!LatteView_IsSiblingValid(texture, slice, mip))
					m_view.pendingSeeds.emplace_back(texture, (uint32)texture->GetSliceMipArrayIndex(slice, mip));
			}
		}
		boundTextures[unit] = (LatteTextureViewVk*)view_getSiblingView(view, false);
	}
	VkDescriptorSetInfo* rightDS = draw_getOrCreateDescriptorSet(pipeline_info, shader);
	for (sint32 i = 0; i < textureCount; i++)
	{
		sint32 unit = shader->resourceMapping.getRelativeTextureUnitFromRelativeBindingPoint(i);
		boundTextures[unit] = leftViews[unit];
	}
	m_view.rightDescriptorSets[leftDS] = { siblingEpoch, rightDS };
	m_view.knownRightDescriptorSets.emplace(rightDS);
	return rightDS;
}

void VulkanRenderer::view_bindRightStage(PipelineInfo* pipeline_info, uint32 stageIndex, VkDescriptorSetInfo* rightDS, LatteDecompilerShader* shader, const float* leftUniformData)
{
	static float s_rightUniformData[512 * 4];
	if (!rightDS)
		return;
	rightDS->m_vkObjDescriptorSet->flagForCurrentCommandBuffer();
	VulkanViewStageBinding binding{};
	binding.set = rightDS->m_vkObjDescriptorSet->descriptorSet;
	binding.dynamicOffsetCount = 0;
	if (pipeline_info->dynamicOffsetInfo.hasUniformVar[stageIndex])
	{
		uint32 leftOffset = dynamicOffsetInfo.uniformVarBufferOffset[stageIndex];
		uint32 rightDataEpoch = LatteView_GetRightDataEpoch();
		VulkanViewUniformData& cached = m_view.uniformData[stageIndex];
		uint64 commandBufferId = GetCurrentCommandBufferId();
		if (cached.shader != shader || cached.leftOffset != leftOffset || cached.rightDataEpoch != rightDataEpoch || cached.commandBufferId != commandBufferId)
		{
			cached.shader = shader;
			cached.commandBufferId = commandBufferId;
			cached.leftOffset = leftOffset;
			cached.rightDataEpoch = rightDataEpoch;
			cached.rightOffset = leftOffset;
			if (uniformData_buildRightView(shader, leftUniformData, s_rightUniformData))
				cached.rightOffset = uniformData_uploadUniformDataBufferGetOffset({(uint8*)s_rightUniformData, shader->uniform.uniformRangeSize});
		}
		binding.dynamicOffsets[binding.dynamicOffsetCount++] = cached.rightOffset;
	}
	if (pipeline_info->dynamicOffsetInfo.hasUniformBuffers[stageIndex])
	{
		for (auto& itr : pipeline_info->dynamicOffsetInfo.list_uniformBuffers[stageIndex])
			binding.dynamicOffsets[binding.dynamicOffsetCount++] = LatteView_GetRightUniformBufferOffset(shader->shaderType, itr);
	}
	VulkanViewStageBinding& bound = m_view.bound.stage[stageIndex];
	if (bound.set == binding.set && bound.dynamicOffsetCount == binding.dynamicOffsetCount && memcmp(bound.dynamicOffsets, binding.dynamicOffsets, binding.dynamicOffsetCount * sizeof(uint32)) == 0)
		return;
	bound = binding;
	VulkanViewCommand& cmd = m_view.commands.emplace_back();
	cmd.type = VulkanViewCommandType::BindDescriptorSet;
	cmd.descriptorSet.layout = pipeline_info->m_vkrObjPipeline->m_pipelineLayout;
	cmd.descriptorSet.setIndex = stageIndex; // vertex, pixel and geometry use sets 0, 1 and 2
	cmd.descriptorSet.set = binding.set;
	cmd.descriptorSet.dynamicOffsetStart = (uint32)m_view.dynamicOffsets.size();
	cmd.descriptorSet.dynamicOffsetCount = (uint32)binding.dynamicOffsetCount;
	m_view.dynamicOffsets.insert(m_view.dynamicOffsets.end(), binding.dynamicOffsets, binding.dynamicOffsets + binding.dynamicOffsetCount);
}

void VulkanRenderer::view_recordDraw(PipelineInfo* pipeline_info, VkDescriptorSetInfo* vertexDS, VkDescriptorSetInfo* pixelDS, VkDescriptorSetInfo* geometryDS, bool isIndexed, uint32 count, uint32 instanceCount, uint32 baseVertex, uint32 baseInstance)
{
	if (LatteGPUState.contextRegister[mmVGT_STRMOUT_EN] != 0)
	{
		// writing stream-out twice would duplicate the output
		cemuLog_logOnce(LogType::Force, "LatteView: Stream-out draws are only issued for the left view");
		return;
	}
	auto& bound = m_view.bound;
	auto emit = [this](VulkanViewCommandType type) -> VulkanViewCommand& {
		VulkanViewCommand& cmd = m_view.commands.emplace_back();
		cmd.type = type;
		return cmd;
	};

	if (bound.pipeline != m_state.currentPipeline)
	{
		bound.pipeline = m_state.currentPipeline;
		emit(VulkanViewCommandType::BindPipeline).pipeline = bound.pipeline;
	}
	VkPipelineLayout layout = pipeline_info->m_vkrObjPipeline->m_pipelineLayout;
	if (bound.layout != layout)
	{
		bound.layout = layout;
		for (auto& stage : bound.stage)
			stage.dynamicOffsetCount = -1;
	}
	if (isIndexed && (bound.indexBuffer != m_view.indexBuffer || bound.indexBufferOffset != m_view.indexBufferOffset || bound.indexType != m_view.indexType))
	{
		bound.indexBuffer = m_view.indexBuffer;
		bound.indexBufferOffset = m_view.indexBufferOffset;
		bound.indexType = m_view.indexType;
		VulkanViewCommand& cmd = emit(VulkanViewCommandType::BindIndexBuffer);
		cmd.indexBuffer.buffer = bound.indexBuffer;
		cmd.indexBuffer.offset = bound.indexBufferOffset;
		cmd.indexBuffer.indexType = bound.indexType;
	}
	if (LatteFetchShader* fetchShader = LatteSHRC_GetActiveFetchShader())
	{
		for (auto& bufferGroup : fetchShader->bufferGroups)
		{
			uint32 index = bufferGroup.attributeBufferIndex;
			auto& binding = m_view.vertexBinding[index];
			if (!binding.buffer || (bound.vertexBuffer[index] == binding.buffer && bound.vertexOffset[index] == binding.offset))
				continue;
			bound.vertexBuffer[index] = binding.buffer;
			bound.vertexOffset[index] = binding.offset;
			VulkanViewCommand& cmd = emit(VulkanViewCommandType::BindVertexBuffer);
			cmd.vertexBuffer.index = index;
			cmd.vertexBuffer.buffer = binding.buffer;
			cmd.vertexBuffer.offset = binding.offset;
		}
	}
	if (memcmp(&bound.viewport, &m_state.currentViewport, sizeof(VkViewport)) != 0)
	{
		bound.viewport = m_state.currentViewport;
		emit(VulkanViewCommandType::SetViewport).viewport = bound.viewport;
	}
	if (memcmp(&bound.scissor, &m_state.currentScissorRect, sizeof(VkRect2D)) != 0)
	{
		bound.scissor = m_state.currentScissorRect;
		emit(VulkanViewCommandType::SetScissor).scissor = bound.scissor;
	}
	if (pipeline_info->usesDepthBias && memcmp(bound.depthBias, m_view.depthBias, sizeof(bound.depthBias)) != 0)
	{
		memcpy(bound.depthBias, m_view.depthBias, sizeof(bound.depthBias));
		memcpy(emit(VulkanViewCommandType::SetDepthBias).depthBias, bound.depthBias, sizeof(bound.depthBias));
	}
	if (pipeline_info->usesBlendConstants && memcmp(bound.blendConstants, m_view.blendConstants, sizeof(bound.blendConstants)) != 0)
	{
		memcpy(bound.blendConstants, m_view.blendConstants, sizeof(bound.blendConstants));
		memcpy(emit(VulkanViewCommandType::SetBlendConstants).blendConstants, bound.blendConstants, sizeof(bound.blendConstants));
	}
	if (UseAttachmentFeedbackLoop() && m_state.feedbackLoopImageAspect != 0xFFFFFFFF && bound.feedbackLoopAspect != m_state.feedbackLoopImageAspect)
	{
		bound.feedbackLoopAspect = m_state.feedbackLoopImageAspect;
		emit(VulkanViewCommandType::SetFeedbackLoop).feedbackLoopAspect = bound.feedbackLoopAspect;
	}

	LatteDecompilerShader* vertexShader = LatteSHRC_GetActiveVertexShader();
	LatteDecompilerShader* pixelShader = LatteSHRC_GetActivePixelShader();
	LatteDecompilerShader* geometryShader = LatteSHRC_GetActiveGeometryShader();
	view_bindRightStage(pipeline_info, VulkanRendererConst::SHADER_STAGE_INDEX_VERTEX, view_getRightDescriptorSet(pipeline_info, vertexShader, vertexDS), vertexShader, VulkanRenderer_GetUniformData(VulkanRendererConst::SHADER_STAGE_INDEX_VERTEX));
	view_bindRightStage(pipeline_info, VulkanRendererConst::SHADER_STAGE_INDEX_FRAGMENT, view_getRightDescriptorSet(pipeline_info, pixelShader, pixelDS), pixelShader, VulkanRenderer_GetUniformData(VulkanRendererConst::SHADER_STAGE_INDEX_FRAGMENT));
	view_bindRightStage(pipeline_info, VulkanRendererConst::SHADER_STAGE_INDEX_GEOMETRY, view_getRightDescriptorSet(pipeline_info, geometryShader, geometryDS), geometryShader, VulkanRenderer_GetUniformData(VulkanRendererConst::SHADER_STAGE_INDEX_GEOMETRY));
	// has to live as long as the command buffer
	m_cmdBufferUniformRingbufIndices[m_commandBufferIndex] = m_uniformVarBufferWriteIndex;

	VulkanViewCommand& cmd = emit(isIndexed ? VulkanViewCommandType::DrawIndexed : VulkanViewCommandType::Draw);
	cmd.draw.count = count;
	cmd.draw.instanceCount = instanceCount;
	cmd.draw.baseVertex = baseVertex;
	cmd.draw.baseInstance = baseInstance;
}

void VulkanRenderer::view_recordBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkDependencyFlags dependencyFlags)
{
	VulkanViewCommand& cmd = m_view.commands.emplace_back();
	cmd.type = VulkanViewCommandType::Barrier;
	cmd.barrier.srcStage = srcStage;
	cmd.barrier.dstStage = dstStage;
	cmd.barrier.srcAccess = srcAccess;
	cmd.barrier.dstAccess = dstAccess;
	cmd.barrier.dependencyFlags = dependencyFlags;
}

void VulkanRenderer::view_flushRightPass()
{
	if (!m_view.isRecording)
		return;
	m_view.isRecording = false;
	bool hasDraws = std::any_of(m_view.commands.begin(), m_view.commands.end(), [](const VulkanViewCommand& cmd) { return cmd.type == VulkanViewCommandType::Draw || cmd.type == VulkanViewCommandType::DrawIndexed; });
	if (!hasDraws)
		return;

	for (auto& seed : m_view.pendingSeeds)
	{
		LatteTexture* texture = seed.first;
		sint32 slice = (sint32)(seed.second % (uint32)texture->depth);
		sint32 mip = (sint32)(seed.second / (uint32)texture->depth);
		if (!LatteView_IsSiblingValid(texture, slice, mip))
			LatteView_SeedSibling(texture, slice, mip);
	}
	m_view.pendingSeeds.clear();

	// right view draws shouldn't count towards the open query
	occlusionQuery_suspendFragments(true);
	if (m_view.rightFBO)
		sync_RenderPassLoadTextures(m_view.rightFBO);
	VkCommandBuffer commandBuffer = m_state.currentCommandBuffer;
	if (m_featureControl.deviceExtensions.dynamic_rendering)
	{
		vkCmdBeginRenderingKHR(commandBuffer, &m_view.renderingInfo);
	}
	else
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_view.renderPass;
		renderPassInfo.framebuffer = m_view.framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_view.extent;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	for (auto& cmd : m_view.commands)
	{
		switch (cmd.type)
		{
		case VulkanViewCommandType::BindPipeline:
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cmd.pipeline);
			break;
		case VulkanViewCommandType::BindIndexBuffer:
			vkCmdBindIndexBuffer(commandBuffer, cmd.indexBuffer.buffer, cmd.indexBuffer.offset, cmd.indexBuffer.indexType);
			break;
		case VulkanViewCommandType::BindVertexBuffer:
			vkCmdBindVertexBuffers(commandBuffer, cmd.vertexBuffer.index, 1, &cmd.vertexBuffer.buffer, &cmd.vertexBuffer.offset);
			break;
		case VulkanViewCommandType::SetViewport:
			vkCmdSetViewport(commandBuffer, 0, 1, &cmd.viewport);
			break;
		case VulkanViewCommandType::SetScissor:
			vkCmdSetScissor(commandBuffer, 0, 1, &cmd.scissor);
			break;
		case VulkanViewCommandType::SetDepthBias:
			vkCmdSetDepthBias(commandBuffer, cmd.depthBias[0], cmd.depthBias[1], cmd.depthBias[2]);
			break;
		case VulkanViewCommandType::SetBlendConstants:
			vkCmdSetBlendConstants(commandBuffer, cmd.blendConstants);
			break;
		case VulkanViewCommandType::SetFeedbackLoop:
			vkCmdSetAttachmentFeedbackLoopEnableEXT(commandBuffer, cmd.feedbackLoopAspect);
			break;
		case VulkanViewCommandType::Barrier:
		{
			VkMemoryBarrier memoryBarrier{};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask = cmd.barrier.srcAccess;
			memoryBarrier.dstAccessMask = cmd.barrier.dstAccess;
			vkCmdPipelineBarrier(commandBuffer, cmd.barrier.srcStage, cmd.barrier.dstStage, cmd.barrier.dependencyFlags, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
			break;
		}
		case VulkanViewCommandType::BindDescriptorSet:
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cmd.descriptorSet.layout, cmd.descriptorSet.setIndex, 1, &cmd.descriptorSet.set, cmd.descriptorSet.dynamicOffsetCount, m_view.dynamicOffsets.data() + cmd.descriptorSet.dynamicOffsetStart);
			break;
		case VulkanViewCommandType::Draw:
			vkCmdDraw(commandBuffer, cmd.draw.count, cmd.draw.instanceCount, cmd.draw.baseVertex, cmd.draw.baseInstance);
			break;
		case VulkanViewCommandType::DrawIndexed:
			vkCmdDrawIndexed(commandBuffer, cmd.draw.count, cmd.draw.instanceCount, 0, (sint32)cmd.draw.baseVertex, cmd.draw.baseInstance);
			break;
		}
	}
	if (m_featureControl.deviceExtensions.dynamic_rendering)
		vkCmdEndRenderingKHR(commandBuffer);
	else
		vkCmdEndRenderPass(commandBuffer);
	if (m_view.rightFBO)
		sync_RenderPassStoreTextures(m_view.rightFBO);
	occlusionQuery_suspendFragments(false);
	m_view.commands.clear();
	m_view.dynamicOffsets.clear();

	HostModule_ResetCommandBufferState();
	m_view.restoreBindings = true;
}

void VulkanRenderer::view_restoreBindings(Renderer::INDEX_TYPE hostIndexType)
{
	m_view.restoreBindings = false;
	if (hostIndexType != Renderer::INDEX_TYPE::NONE && m_view.indexBuffer)
		vkCmdBindIndexBuffer(m_state.currentCommandBuffer, m_view.indexBuffer, m_view.indexBufferOffset, m_view.indexType);
	if (LatteFetchShader* fetchShader = LatteSHRC_GetActiveFetchShader())
	{
		for (auto& bufferGroup : fetchShader->bufferGroups)
		{
			auto& binding = m_view.vertexBinding[bufferGroup.attributeBufferIndex];
			if (binding.buffer)
				vkCmdBindVertexBuffers(m_state.currentCommandBuffer, bufferGroup.attributeBufferIndex, 1, &binding.buffer, &binding.offset);
		}
	}
}
