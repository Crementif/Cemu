#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanRenderer.h"
#include "Cafe/HW/Latte/Renderer/Vulkan/LatteTextureVk.h"
#include "Cafe/HW/Latte/Renderer/Vulkan/LatteTextureViewVk.h"
#include "Cafe/HostModule/HostModule.h"
#include "Cafe/HW/Latte/Core/LatteView.h"

VkResult VulkanRenderer::HostModule_CreateInstance(const VkInstanceCreateInfo& createInfo, VkInstance& instance, bool& createdByModule)
{
	createdByModule = false;
	for (auto& module : HostModule::GetModules())
	{
		VkResult result = VK_ERROR_INITIALIZATION_FAILED;
		if (module->CreateVkInstance(createInfo, instance, result))
		{
			createdByModule = true;
			cemuLog_log(LogType::Force, "Vulkan instance created by host module {} ({})", module->GetName(), (sint32)result);
			return result;
		}
	}
	return vkCreateInstance(&createInfo, nullptr, &instance);
}

VkPhysicalDevice VulkanRenderer::HostModule_SelectPhysicalDevice(VkInstance instance)
{
	for (auto& module : HostModule::GetModules())
	{
		VkPhysicalDevice device = module->SelectPhysicalDevice(instance);
		if (device != VK_NULL_HANDLE)
			return device;
	}
	return VK_NULL_HANDLE;
}

VkResult VulkanRenderer::HostModule_CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& createInfo, VkDevice& device)
{
	for (auto& module : HostModule::GetModules())
	{
		VkResult result = VK_ERROR_INITIALIZATION_FAILED;
		if (module->CreateVkDevice(physicalDevice, createInfo, device, result))
		{
			cemuLog_log(LogType::Force, "Vulkan device created by host module {} ({})", module->GetName(), (sint32)result);
			return result;
		}
	}
	return vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
}

void VulkanRenderer::HostModule_NotifyDeviceCreated()
{
	HostModule::VulkanDevice device{};
	device.getInstanceProcAddr = vkGetInstanceProcAddr;
	device.instance = m_instance;
	device.physicalDevice = m_physicalDevice;
	device.device = m_logicalDevice;
	device.graphicsQueueFamily = m_indices.graphicsFamily;
	for (auto& module : HostModule::GetModules())
		module->OnVkDeviceCreated(device);
}

void VulkanRenderer::HostModule_NotifyDeviceDestroying()
{
	for (auto& module : HostModule::GetModules())
		module->OnVkDeviceDestroying();
}

void VulkanRenderer::HostModule_AttachSubmitSemaphores(VkSubmitInfo& submitInfo, VkTimelineSemaphoreSubmitInfo& timelineInfo)
{
	if (!HostModule::HasModules())
		return;
	static thread_local std::vector<HostModule::SubmitSemaphore> s_waits, s_signals;
	HostModule::TakeSubmitSemaphores(s_waits, s_signals);
	if (s_waits.empty() && s_signals.empty())
		return;

	static thread_local std::vector<VkSemaphore> s_waitSemaphores, s_signalSemaphores;
	static thread_local std::vector<VkPipelineStageFlags> s_waitStages;
	static thread_local std::vector<uint64> s_waitValues, s_signalValues;
	s_waitSemaphores.assign(submitInfo.pWaitSemaphores, submitInfo.pWaitSemaphores + submitInfo.waitSemaphoreCount);
	s_waitStages.assign(submitInfo.pWaitDstStageMask, submitInfo.pWaitDstStageMask + submitInfo.waitSemaphoreCount);
	s_waitValues.assign(submitInfo.waitSemaphoreCount, 0);
	s_signalSemaphores.assign(submitInfo.pSignalSemaphores, submitInfo.pSignalSemaphores + submitInfo.signalSemaphoreCount);
	s_signalValues.assign(submitInfo.signalSemaphoreCount, 0);
	for (auto& wait : s_waits)
	{
		s_waitSemaphores.emplace_back(wait.semaphore);
		s_waitStages.emplace_back(VK_PIPELINE_STAGE_TRANSFER_BIT);
		s_waitValues.emplace_back(wait.value);
	}
	for (auto& signal : s_signals)
	{
		s_signalSemaphores.emplace_back(signal.semaphore);
		s_signalValues.emplace_back(signal.value);
	}

	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = submitInfo.pNext;
	timelineInfo.waitSemaphoreValueCount = (uint32)s_waitValues.size();
	timelineInfo.pWaitSemaphoreValues = s_waitValues.data();
	timelineInfo.signalSemaphoreValueCount = (uint32)s_signalValues.size();
	timelineInfo.pSignalSemaphoreValues = s_signalValues.data();
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = (uint32)s_waitSemaphores.size();
	submitInfo.pWaitSemaphores = s_waitSemaphores.data();
	submitInfo.pWaitDstStageMask = s_waitStages.data();
	submitInfo.signalSemaphoreCount = (uint32)s_signalSemaphores.size();
	submitInfo.pSignalSemaphores = s_signalSemaphores.data();
}

// the module might have changed any state
void VulkanRenderer::HostModule_ResetCommandBufferState()
{
	m_state.resetCommandBufferState();
	m_state.descriptorSetsChanged = true;
	vkCmdSetViewport(m_state.currentCommandBuffer, 0, 1, &m_state.currentViewport);
	vkCmdSetScissor(m_state.currentCommandBuffer, 0, 1, &m_state.currentScissorRect);
}

static HostModule::EventSurface HostModule_GetSurface(LatteTextureVk* texture, uint32 sliceIndex)
{
	sint32 width, height;
	texture->GetEffectiveSize(width, height, 0);
	HostModule::EventSurface surface{};
	surface.image = texture->GetImageObj()->m_image;
	surface.layout = VK_IMAGE_LAYOUT_GENERAL;
	surface.aspectMask = texture->GetImageAspect();
	surface.format = texture->GetFormat();
	surface.extent = { (uint32)width, (uint32)height };
	surface.sliceIndex = sliceIndex;
	return surface;
}

void VulkanRenderer::HostModule_DispatchEvent(const HostModule::HostEvent& event, LatteTexture* texture, uint32 sliceIndex)
{
	draw_endRenderPass();

	HostModule::HostEvent moduleEvent = event;
	HostModule::EventSurface surface{};
	HostModule::EventSurface rightSurface{};
	LatteTextureVk* vkTexture = (LatteTextureVk*)texture;
	LatteTextureVk* vkRightTexture = nullptr;
	if (texture && LatteView_IsStereo() && LatteView_IsSiblingValid(texture, sliceIndex, 0))
		vkRightTexture = (LatteTextureVk*)texture->viewSibling;
	VkImageSubresourceLayers subresource{};
	moduleEvent.surface = nullptr;
	moduleEvent.rightSurface = nullptr;
	if (vkTexture)
	{
		subresource.aspectMask = vkTexture->GetImageAspect();
		subresource.mipLevel = 0;
		subresource.baseArrayLayer = sliceIndex;
		subresource.layerCount = 1;
		vkTexture->GetImageObj()->flagForCurrentCommandBuffer();
		barrier_image<ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE, ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE>(vkTexture, subresource, VK_IMAGE_LAYOUT_GENERAL);
		surface = HostModule_GetSurface(vkTexture, sliceIndex);
		moduleEvent.surface = &surface;
		if (vkRightTexture)
		{
			vkRightTexture->GetImageObj()->flagForCurrentCommandBuffer();
			barrier_image<ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE, ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE>(vkRightTexture, subresource, VK_IMAGE_LAYOUT_GENERAL);
			rightSurface = HostModule_GetSurface(vkRightTexture, sliceIndex);
			moduleEvent.rightSurface = &rightSurface;
		}
	}

	for (auto& module : HostModule::GetModules())
		module->OnHostEvent(moduleEvent, m_state.currentCommandBuffer);

	if (vkTexture)
		barrier_image<ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE, ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE>(vkTexture, subresource, vkTexture->GetDefaultLayout());
	if (vkRightTexture)
		barrier_image<ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE, ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE>(vkRightTexture, subresource, vkRightTexture->GetDefaultLayout());
	HostModule_ResetCommandBufferState();
}

void VulkanRenderer::HostModule_DeliverScanBuffer(LatteTextureView* textureView, bool padView)
{
	draw_endRenderPass();

	LatteTextureVk* vkTexture = (LatteTextureVk*)textureView->baseTexture;
	vkTexture->GetImageObj()->flagForCurrentCommandBuffer();
	VkImageSubresourceLayers subresource{};
	subresource.aspectMask = vkTexture->GetImageAspect();
	subresource.mipLevel = textureView->firstMip;
	subresource.baseArrayLayer = textureView->firstSlice;
	subresource.layerCount = 1;
	barrier_image<ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE, ANY_TRANSFER | IMAGE_READ>(vkTexture, subresource, VK_IMAGE_LAYOUT_GENERAL);
	HostModule::EventSurface surface = HostModule_GetSurface(vkTexture, textureView->firstSlice);

	for (auto& module : HostModule::GetModules())
		module->OnScanBuffer(surface, padView, m_state.currentCommandBuffer);

	barrier_image<ANY_TRANSFER | IMAGE_READ, ANY_TRANSFER | IMAGE_READ | IMAGE_WRITE>(vkTexture, subresource, vkTexture->GetDefaultLayout());
	HostModule_ResetCommandBufferState();
}
