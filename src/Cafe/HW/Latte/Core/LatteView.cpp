#include "Cafe/HW/Latte/Core/LatteView.h"
#include "Cafe/HW/Latte/Core/Latte.h"
#include "Cafe/HW/Latte/Core/LatteTexture.h"
#include "Cafe/HW/Latte/Core/LattePM4.h"
#include "Cafe/HW/Latte/ISA/LatteReg.h"
#include "Cafe/HW/Latte/Renderer/Renderer.h"
#include "Cafe/HW/Latte/Core/LatteBufferCache.h"
#include "Common/SysAllocator.h"

#ifdef ENABLE_VULKAN
#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanRenderer.h"
#endif

enum class ViewScopeMode : uint32
{
	Mono = 0,
	Stereo = 1,
	MonoSectionBegin = 2,
	MonoSectionEnd = 3,
};

static bool s_isStereo = false;
static bool s_isMonoSection = false;
static bool s_multiviewRequested = false;
static std::optional<LatteViewFoveation> s_foveation;
static bool s_foveationActive = true;
static FSpinlock s_foveationFocusLock;
static LatteViewFoveationFocus s_foveationFocus = { { { 0.5f, 0.5f }, { 0.5f, 0.5f } } };
static uint32 s_foveationFocusEpoch = 0;
static uint32 s_renderTargetCreationDepth = 0;

struct RightWords
{
	MPTR physAddr;
	std::vector<uint32be> words;
};
static std::vector<RightWords> s_rightWords;
// ranges of the right words, to quickly check if a block has any
static std::vector<std::pair<MPTR, MPTR>> s_rightWordRanges;

// uniform blocks the right view looked up in this scope, data is empty if it has no right words
struct RightBlock
{
	uint32 size;
	std::vector<uint8> data;
	MPTR copyPhysAddr;
	std::vector<std::pair<uint32, MPTR>> pairCopies;
};
static std::unordered_map<MPTR, RightBlock> s_rightBlocks;

static constexpr uint32 RIGHT_BLOCK_ARENA_SIZE = 2 * 1024 * 1024;
static SysAllocator<uint8, RIGHT_BLOCK_ARENA_SIZE, 256> s_rightBlockArena;
static uint32 s_rightBlockArenaUsed = 0;

static uint32 s_rightUniformBufferOffset[3][LATTE_NUM_MAX_UNIFORM_BUFFERS];

static uint32 s_rightAluConst[0x800];
static std::bitset<512> s_rightAluConstMask;
static uint32 s_rightAluConstCount[2];

static uint32 s_siblingEpoch = 1;
static uint32 s_rightDataEpoch = 1;

LatteTexture* LatteView_GetOrCreateSibling(LatteTexture* texture)
{
	if (texture->isViewSibling)
		return nullptr;
	if (texture->viewSibling)
		return texture->viewSibling;
	if (texture->hasViewLayer)
	{
		texture->viewSibling = texture;
		texture->viewSiblingValid.assign(texture->GetSliceMipArraySize(), false);
		s_siblingEpoch++;
		return texture;
	}
	uint32 renderTargetCreationDepth = std::exchange(s_renderTargetCreationDepth, 0);
	LatteTexture* sibling = g_renderer->texture_createTextureEx(texture->dim, texture->physAddress, texture->physMipAddress, texture->format, texture->width, texture->height, texture->depth, texture->pitch, texture->mipLevels, texture->swizzle, texture->tileMode, texture->isDepth);
	sibling->isViewSibling = true;
	// texture rules could have changed since, and a size mismatch would overrun copies
	sint32 width, height, siblingWidth, siblingHeight;
	texture->GetEffectiveSize(width, height, 0);
	sibling->GetEffectiveSize(siblingWidth, siblingHeight, 0);
	if (width != siblingWidth || height != siblingHeight)
	{
		cemuLog_logOnce(LogType::Force, "LatteView: Texture {:08x} no longer matches its texture rules, the right view shares it", texture->physAddress);
		delete sibling;
		return nullptr;
	}
	s_renderTargetCreationDepth = renderTargetCreationDepth;
	LatteTexture_InitSliceAndMipInfo(sibling);
	sibling->AllocateOnHost();
	sibling->isDataDefined = true;
	sibling->baseView = sibling->GetOrCreateView(0, sibling->mipLevels, 0, sibling->depth);
	texture->viewSibling = sibling;
	texture->viewSiblingValid.assign(texture->GetSliceMipArraySize(), false);
	s_siblingEpoch++;
	return sibling;
}

void LatteView_DeleteSibling(LatteTexture* texture)
{
	LatteTexture* sibling = texture->viewSibling;
	if (!sibling)
		return;
	if (sibling == texture)
	{
		texture->viewSibling = nullptr;
		texture->viewSiblingValid.clear();
		s_siblingEpoch++;
		return;
	}
#ifdef ENABLE_VULKAN
	if (g_renderer && g_renderer->GetType() == RendererAPI::Vulkan)
		VulkanRenderer::GetInstance()->view_notifySiblingDeleted(texture);
#endif
	texture->viewSibling = nullptr;
	texture->viewSiblingValid.clear();
	while (!sibling->views.empty())
		delete sibling->views[0];
	delete[] sibling->sliceMipInfo;
	sibling->sliceMipInfo = nullptr;
	delete sibling;
	s_siblingEpoch++;
}

sint32 LatteView_GetSiblingSlice(LatteTexture* texture, sint32 sliceIndex)
{
	return texture->viewSibling == texture ? sliceIndex + texture->depth : sliceIndex;
}

bool LatteView_SupportsViewLayer(Latte::E_DIM dim, uint32 depth)
{
	if (dim != Latte::E_DIM::DIM_2D && dim != Latte::E_DIM::DIM_2D_ARRAY)
		return false;
#ifdef ENABLE_VULKAN
	if (!LatteView_UsesMultiview(false) || depth >= VulkanRenderer::GetInstance()->GetMaxMultiviewViewCount())
		return false;
	return true;
#else
	return false;
#endif
}

void LatteView_BeginRenderTargetCreation()
{
	s_renderTargetCreationDepth++;
}

void LatteView_EndRenderTargetCreation()
{
	s_renderTargetCreationDepth--;
}

bool LatteView_CreatesViewLayer(Latte::E_DIM dim, uint32 depth)
{
	return s_renderTargetCreationDepth != 0 && LatteView_SupportsViewLayer(dim, depth);
}

bool LatteView_LacksViewLayer(LatteTexture* texture)
{
	return !texture->hasViewLayer && !texture->viewLayerUnavailable && !texture->isViewSibling && LatteView_SupportsViewLayer(texture->dim, texture->depth);
}

bool LatteView_IsSiblingValid(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex)
{
	if (!texture->viewSibling || sliceIndex >= texture->depth || mipIndex >= texture->mipLevels)
		return false;
	return texture->viewSiblingValid[texture->GetSliceMipArrayIndex(sliceIndex, mipIndex)];
}

void LatteView_SetSiblingValid(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex, bool isValid)
{
	if (!texture->viewSibling || sliceIndex >= texture->depth || mipIndex >= texture->mipLevels)
		return;
	sint32 index = texture->GetSliceMipArrayIndex(sliceIndex, mipIndex);
	if (texture->viewSiblingValid[index] == isValid)
		return;
	texture->viewSiblingValid[index] = isValid;
	s_siblingEpoch++;
}

bool LatteView_HasValidSiblingInRange(LatteTexture* texture, sint32 firstSlice, sint32 numSlice, sint32 firstMip, sint32 numMip)
{
	if (!texture->viewSibling)
		return false;
	for (sint32 mip = firstMip; mip < std::min(firstMip + numMip, texture->mipLevels); mip++)
	{
		for (sint32 slice = firstSlice; slice < std::min(firstSlice + numSlice, texture->depth); slice++)
		{
			if (texture->viewSiblingValid[texture->GetSliceMipArrayIndex(slice, mip)])
				return true;
		}
	}
	return false;
}

uint32 LatteView_GetSiblingEpoch()
{
	return s_siblingEpoch;
}

// the sibling is empty until it's seeded, so partial writes need the texture's data first
void LatteView_SeedSibling(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex)
{
	sint32 width, height;
	texture->GetEffectiveSize(width, height, mipIndex);
	g_renderer->texture_copyImageSubData(texture, mipIndex, 0, 0, sliceIndex, texture->viewSibling, mipIndex, 0, 0, LatteView_GetSiblingSlice(texture, sliceIndex), width, height, 1);
	LatteView_SetSiblingValid(texture, sliceIndex, mipIndex, true);
}

LatteTexture* LatteView_OnTextureClear(LatteTexture* texture, sint32 sliceIndex, sint32 mipIndex)
{
	if (texture->isViewSibling || sliceIndex >= texture->depth)
		return nullptr;
	// a clear is the same for both views
	LatteView_SetSiblingValid(texture, sliceIndex, mipIndex, false);
	return nullptr;
}

bool LatteView_OnTextureCopy(LatteTexture* src, sint32 srcSlice, sint32 srcMip, LatteTexture* dst, sint32 dstSlice, sint32 dstMip, LatteTexture*& srcSibling, LatteTexture*& dstSibling)
{
	if (src->isViewSibling || dst->isViewSibling || srcSlice >= src->depth || dstSlice >= dst->depth)
		return false;
	if (!s_isStereo || s_isMonoSection || !LatteView_IsSiblingValid(src, srcSlice, srcMip))
	{
		LatteView_SetSiblingValid(dst, dstSlice, dstMip, false);
		return false;
	}
	dstSibling = LatteView_GetOrCreateSibling(dst);
	if (!dstSibling)
		return false;
	if (!LatteView_IsSiblingValid(dst, dstSlice, dstMip))
		LatteView_SeedSibling(dst, dstSlice, dstMip);
	srcSibling = src->viewSibling;
	return true;
}

static void LatteView_resetScopeState()
{
	s_isMonoSection = false;
	s_rightDataEpoch++;
	s_rightWords.clear();
	s_rightWordRanges.clear();
	s_rightBlocks.clear();
	s_rightBlockArenaUsed = 0;
	s_rightAluConstMask.reset();
	s_rightAluConstCount[0] = 0;
	s_rightAluConstCount[1] = 0;
}

void LatteView_SetMultiviewRequested(bool requested)
{
	s_multiviewRequested = requested;
}

bool LatteView_IsMultiviewRequested()
{
	return s_multiviewRequested;
}

bool LatteView_UsesMultiview(bool withGeometryShader)
{
#ifdef ENABLE_VULKAN
	if (!s_multiviewRequested || !g_renderer || g_renderer->GetType() != RendererAPI::Vulkan)
		return false;
	return VulkanRenderer::GetInstance()->SupportsMultiview(withGeometryShader);
#else
	return false;
#endif
}

void LatteView_RequestFoveation(const LatteViewFoveation& foveation)
{
	s_foveation = foveation;
	LatteView_SetFoveationRadii(foveation.innerRadius, foveation.outerRadius);
}

const LatteViewFoveation* LatteView_GetFoveation()
{
	return s_foveation ? &*s_foveation : nullptr;
}

void LatteView_SetFoveationCentres(const float left[2], const float right[2])
{
	auto isClose = [](const float a[2], const float b[2]) { return std::abs(a[0] - b[0]) < 0.002f && std::abs(a[1] - b[1]) < 0.002f; };
	std::unique_lock _l(s_foveationFocusLock);
	if (isClose(s_foveationFocus.centres[0], left) && isClose(s_foveationFocus.centres[1], right))
		return;
	std::copy_n(left, 2, s_foveationFocus.centres[0]);
	std::copy_n(right, 2, s_foveationFocus.centres[1]);
	s_foveationFocusEpoch++;
}

void LatteView_SetFoveationRadii(float innerRadius, float outerRadius)
{
	std::unique_lock _l(s_foveationFocusLock);
	if (s_foveationFocus.innerRadius == innerRadius && s_foveationFocus.outerRadius == outerRadius)
		return;
	s_foveationFocus.innerRadius = innerRadius;
	s_foveationFocus.outerRadius = outerRadius;
	s_foveationFocusEpoch++;
}

uint32 LatteView_GetFoveationFocus(LatteViewFoveationFocus& focus)
{
	std::unique_lock _l(s_foveationFocusLock);
	focus = s_foveationFocus;
	return s_foveationFocusEpoch;
}

void LatteView_SetFoveationActive(bool active)
{
	s_foveationActive = active;
}

bool LatteView_IsFoveationActive()
{
	return s_foveationActive && s_foveation;
}

bool LatteView_IsStereo()
{
	return s_isStereo;
}

bool LatteView_IsMonoSection()
{
	return s_isMonoSection;
}

void LatteView_itViewScope(const uint32be* data, uint32 nWords)
{
	cemu_assert_debug(nWords == 1);
	const ViewScopeMode mode = nWords >= 1 ? (ViewScopeMode)(uint32)data[0] : ViewScopeMode::Mono;
	if (mode == ViewScopeMode::MonoSectionBegin || mode == ViewScopeMode::MonoSectionEnd)
	{
		bool isMonoSection = mode == ViewScopeMode::MonoSectionBegin;
		if (!s_isStereo || isMonoSection == s_isMonoSection)
			return;
#ifdef ENABLE_VULKAN
		VulkanRenderer::GetInstance()->view_setMonoSection();
#endif
		s_isMonoSection = isMonoSection;
		return;
	}
	bool isStereo = mode == ViewScopeMode::Stereo;
#ifdef ENABLE_VULKAN
	VulkanRenderer* vkRenderer = (g_renderer && g_renderer->GetType() == RendererAPI::Vulkan) ? VulkanRenderer::GetInstance() : nullptr;
	if (!vkRenderer)
		isStereo = false;
#else
	isStereo = false;
#endif
	if (isStereo == s_isStereo)
		return;
#ifdef ENABLE_VULKAN
	vkRenderer->view_setStereo(isStereo);
#endif
	s_isStereo = isStereo;
	LatteView_resetScopeState();
}

static void LatteView_addRightWordRange(MPTR begin, MPTR end)
{
	auto it = std::lower_bound(s_rightWordRanges.begin(), s_rightWordRanges.end(), begin, [](const std::pair<MPTR, MPTR>& range, MPTR address) { return range.second < address; });
	auto last = it;
	while (last != s_rightWordRanges.end() && last->first <= end)
	{
		begin = std::min(begin, last->first);
		end = std::max(end, last->second);
		++last;
	}
	it = s_rightWordRanges.erase(it, last);
	s_rightWordRanges.insert(it, { begin, end });
}

static bool LatteView_overlapsRightWords(MPTR begin, MPTR end)
{
	auto it = std::upper_bound(s_rightWordRanges.begin(), s_rightWordRanges.end(), begin, [](MPTR address, const std::pair<MPTR, MPTR>& range) { return address < range.second; });
	return it != s_rightWordRanges.end() && it->first < end;
}

void LatteView_itViewMemWrite(const uint32be* data, uint32 nWords)
{
	if (!s_isStereo || nWords < 2)
		return;
	MPTR physAddr = (MPTR)data[0] & ~3;
	s_rightDataEpoch++;
	RightWords& entry = s_rightWords.emplace_back();
	entry.physAddr = physAddr;
	entry.words.assign(data + 1, data + nWords);
	LatteView_addRightWordRange(physAddr, physAddr + (nWords - 1) * 4);
	for (auto it = s_rightBlocks.begin(); it != s_rightBlocks.end();)
	{
		uint32 wordsEnd = physAddr + (nWords - 1) * 4;
		if (physAddr < it->first + it->second.size && wordsEnd > it->first)
			it = s_rightBlocks.erase(it);
		else
			++it;
	}
}

bool LatteView_HasRightWords()
{
	return !s_rightWords.empty();
}

uint32 LatteView_GetRightDataEpoch()
{
	return s_rightDataEpoch;
}

const uint8* LatteView_GetRightUniformBlock(MPTR physAddr, uint32 size)
{
	if (s_rightWords.empty() || physAddr == MPTR_NULL || !LatteView_overlapsRightWords(physAddr, physAddr + size))
		return nullptr;
	auto it = s_rightBlocks.find(physAddr);
	if (it != s_rightBlocks.end() && it->second.size >= size)
		return it->second.data.empty() ? nullptr : it->second.data.data();

	RightBlock& block = s_rightBlocks[physAddr];
	block.size = size;
	block.data.clear();
	block.copyPhysAddr = MPTR_NULL;
	MPTR blockEnd = physAddr + size;
	for (auto& entry : s_rightWords)
	{
		MPTR wordsEnd = entry.physAddr + (uint32)entry.words.size() * 4;
		if (entry.physAddr >= blockEnd || wordsEnd <= physAddr)
			continue;
		if (block.data.empty())
		{
			block.data.resize(size);
			memcpy(block.data.data(), memory_getPointerFromPhysicalOffset(physAddr), size);
		}
		for (size_t i = 0; i < entry.words.size(); i++)
		{
			MPTR wordAddr = entry.physAddr + (uint32)i * 4;
			if (wordAddr < physAddr || wordAddr + 4 > blockEnd)
				continue;
			memcpy(block.data.data() + (wordAddr - physAddr), &entry.words[i], 4);
		}
	}
	return block.data.empty() ? nullptr : block.data.data();
}

static MPTR LatteView_allocateArena(uint32 size)
{
	uint32 alignedSize = (size + 0xFF) & ~0xFF;
	if (s_rightBlockArenaUsed + alignedSize > RIGHT_BLOCK_ARENA_SIZE)
	{
		cemuLog_logOnce(LogType::Force, "LatteView: Out of memory for right-view uniform buffers, the right view reads the left ones");
		return MPTR_NULL;
	}
	MPTR physAddr = memory_virtualToPhysical(s_rightBlockArena.GetMPTR()) + s_rightBlockArenaUsed;
	s_rightBlockArenaUsed += alignedSize;
	return physAddr;
}

MPTR LatteView_GetUniformBufferPair(MPTR physAddr, uint32 size, uint32 stride)
{
	if (!s_isStereo || size > stride)
		return MPTR_NULL;
	const uint8* rightData = LatteView_GetRightUniformBlock(physAddr, size);
	if (!rightData)
		return MPTR_NULL;
	RightBlock& block = s_rightBlocks[physAddr];
	for (auto& it : block.pairCopies)
	{
		if (it.first == stride)
			return it.second;
	}
	MPTR pairPhysAddr = LatteView_allocateArena(stride + size);
	if (pairPhysAddr == MPTR_NULL)
		return MPTR_NULL;
	uint8* pair = memory_getPointerFromPhysicalOffset(pairPhysAddr);
	memcpy(pair, memory_getPointerFromPhysicalOffset(physAddr), size);
	memcpy(pair + stride, rightData, size);
	LatteBufferCache_invalidate(pairPhysAddr, stride + size);
	block.pairCopies.emplace_back(stride, pairPhysAddr);
	return pairPhysAddr;
}

static MPTR LatteView_getRightUniformBlockCopy(MPTR physAddr, uint32 size)
{
	if (!LatteView_GetRightUniformBlock(physAddr, size))
		return MPTR_NULL;
	RightBlock& block = s_rightBlocks[physAddr];
	if (block.copyPhysAddr != MPTR_NULL)
		return block.copyPhysAddr;
	MPTR copyPhysAddr = LatteView_allocateArena(block.size);
	if (copyPhysAddr == MPTR_NULL)
		return MPTR_NULL;
	memcpy(memory_getPointerFromPhysicalOffset(copyPhysAddr), block.data.data(), block.size);
	LatteBufferCache_invalidate(copyPhysAddr, block.size);
	block.copyPhysAddr = copyPhysAddr;
	return copyPhysAddr;
}

void LatteView_SyncRightUniformBuffer(LatteConst::ShaderType shaderType, uint32 bufferIndex, MPTR physAddr, uint32 size, uint32 bindOffset)
{
	uint32& rightOffset = s_rightUniformBufferOffset[(uint32)shaderType][bufferIndex];
	rightOffset = bindOffset;
	if (!s_isStereo || s_rightWords.empty() || physAddr == MPTR_NULL)
		return;
	if (MPTR copyPhysAddr = LatteView_getRightUniformBlockCopy(physAddr, size); copyPhysAddr != MPTR_NULL)
		rightOffset = LatteBufferCache_retrieveDataInCache(copyPhysAddr, size);
}

uint32 LatteView_GetRightUniformBufferOffset(LatteConst::ShaderType shaderType, uint32 bufferIndex)
{
	return s_rightUniformBufferOffset[(uint32)shaderType][bufferIndex];
}

void LatteView_SetRightAluConst(uint32 registerOffset, uint32 count, const uint32be* values)
{
	if (!s_isStereo || registerOffset + count > 0x800)
		return;
	s_rightDataEpoch++;
	const uint32* leftAluConst = LatteGPUState.contextRegister + LATTE_REG_BASE_ALU_CONST;
	for (uint32 vec4 = registerOffset / 4; vec4 < (registerOffset + count + 3) / 4; vec4++)
	{
		if (s_rightAluConstMask.test(vec4))
			continue;
		memcpy(s_rightAluConst + vec4 * 4, leftAluConst + vec4 * 4, 16);
		s_rightAluConstMask.set(vec4);
		s_rightAluConstCount[vec4 >= 256 ? 1 : 0]++;
	}
	for (uint32 i = 0; i < count; i++)
		s_rightAluConst[registerOffset + i] = values[i];
}

void LatteView_ClearRightAluConst(uint32 registerOffset, uint32 count)
{
	if (s_rightAluConstCount[0] == 0 && s_rightAluConstCount[1] == 0)
		return;
	const uint32* leftAluConst = LatteGPUState.contextRegister + LATTE_REG_BASE_ALU_CONST;
	uint32 registerEnd = std::min<uint32>(registerOffset + count, 0x800);
	for (uint32 vec4 = registerOffset / 4; vec4 * 4 < registerEnd; vec4++)
	{
		if (!s_rightAluConstMask.test(vec4))
			continue;
		s_rightDataEpoch++;
		if (registerOffset <= vec4 * 4 && registerEnd >= vec4 * 4 + 4)
		{
			s_rightAluConstMask.reset(vec4);
			s_rightAluConstCount[vec4 >= 256 ? 1 : 0]--;
			continue;
		}
		for (uint32 i = std::max(registerOffset, vec4 * 4); i < std::min(registerEnd, vec4 * 4 + 4); i++)
			s_rightAluConst[i] = leftAluConst[i];
	}
}

void LatteView_itSetAluConst(const uint32be* data, uint32 nWords)
{
	if (s_rightAluConstCount[0] == 0 && s_rightAluConstCount[1] == 0)
		return;
	if (nWords >= 2)
		LatteView_ClearRightAluConst(data[0], nWords - 1);
}

void LatteView_itLoadAluConst(const uint32be* data, uint32 nWords)
{
	if (s_rightAluConstCount[0] == 0 && s_rightAluConstCount[1] == 0)
		return;
	if (nWords < 2 || (nWords & 1) != 0)
		return;
	for (uint32 i = 2; i + 1 < nWords; i += 2)
		LatteView_ClearRightAluConst(data[i], data[i + 1]);
}

bool LatteView_HasRightAluConst(bool isVertexStage)
{
	return s_rightAluConstCount[isVertexStage ? 1 : 0] != 0;
}

bool LatteView_IsRightAluConstVec4(uint32 vec4Index)
{
	return s_rightAluConstMask.test(vec4Index);
}

const uint32* LatteView_GetRightAluConst()
{
	return s_rightAluConst;
}
