#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanRenderer.h"
#include "Cafe/HW/Latte/Core/LatteView.h"

// (log2 width << 2) | log2 height
static constexpr uint8 RATE_1X1 = 0;
static constexpr uint8 RATE_2X2 = (1 << 2) | 1;
static constexpr uint8 RATE_4X4 = (2 << 2) | 2;

bool VulkanRenderer::foveation_appliesTo(const LatteCachedFBO& fbo, uint32 viewMask) const
{
	const LatteViewFoveation* foveation = LatteView_GetFoveation();
	if (!foveation || !m_featureControl.fragmentShadingRate.supported)
		return false;
	if (viewMask != 0 && !m_featureControl.fragmentShadingRate.layered)
		return false;
	auto hasGuestSize = [foveation](const LatteTextureView* view) {
		return view && view->firstMip == 0 && (uint32)view->baseTexture->width == foveation->guestWidth && (uint32)view->baseTexture->height == foveation->guestHeight;
	};
	for (auto& colorBuffer : fbo.colorBuffer)
	{
		if (colorBuffer.texture)
			return hasGuestSize(colorBuffer.texture);
	}
	return hasGuestSize(fbo.depthBuffer.texture);
}

VKRObjectTextureView* VulkanRenderer::foveation_getRateView(Vector2i size, uint32 viewMask, FoveationPattern pattern)
{
	const VkExtent2D texelSize = GetShadingRateTexelSize();
	const VkExtent2D extent = { ((uint32)size.x + texelSize.width - 1) / texelSize.width, ((uint32)size.y + texelSize.height - 1) / texelSize.height };
	for (auto& rateImage : m_foveation.rateImages)
	{
		if (rateImage->extent.width == extent.width && rateImage->extent.height == extent.height && rateImage->viewMask == viewMask)
			return rateImage->views[(size_t)pattern];
	}

	auto& rateImage = m_foveation.rateImages.emplace_back(std::make_unique<VulkanFoveationRateImage>());
	const uint32 viewLayers = viewMask != 0 ? 32 - std::countl_zero(viewMask) : 1;
	rateImage->extent = extent;
	rateImage->viewMask = viewMask;
	rateImage->layerCount = viewMask != 0 ? viewLayers * 2 : (uint32)FoveationPattern::Count;
	rateImage->focusEpoch = ~0u;
	rateImage->isInitialized = false;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8_UINT;
	imageInfo.extent = { extent.width, extent.height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = rateImage->layerCount;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	rateImage->image = new VKRObjectTexture();
	rateImage->image->m_format = imageInfo.format;
	rateImage->image->m_flags = 0;
	rateImage->image->m_imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	if (vkCreateImage(m_logicalDevice, &imageInfo, nullptr, &rateImage->image->m_image) != VK_SUCCESS)
		UnrecoverableError("Failed to create a shading rate image");
	rateImage->image->m_allocation = memoryManager->imageMemoryAllocate(rateImage->image->m_image);

	for (size_t i = 0; i < rateImage->views.size(); i++)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = rateImage->image->m_image;
		viewInfo.format = imageInfo.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		if (viewMask != 0)
		{
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			viewInfo.subresourceRange.baseArrayLayer = (FoveationPattern)i == FoveationPattern::Full ? viewLayers : 0;
			viewInfo.subresourceRange.layerCount = viewLayers;
		}
		else
		{
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.subresourceRange.baseArrayLayer = (uint32)i;
			viewInfo.subresourceRange.layerCount = 1;
		}
		VkImageView view;
		if (vkCreateImageView(m_logicalDevice, &viewInfo, nullptr, &view) != VK_SUCCESS)
			UnrecoverableError("Failed to create a shading rate image view");
		rateImage->views[i] = new VKRObjectTextureView(rateImage->image, view);
	}
	m_foveation.hasNewImage = true;
	return rateImage->views[(size_t)pattern];
}

FoveationPattern VulkanRenderer::foveation_getPattern(bool isRightPass) const
{
	if (!LatteView_IsFoveationActive() || !m_view.isStereo || LatteView_IsMonoSection())
		return FoveationPattern::Full;
	if (isRightPass)
		return FoveationPattern::Right;
	if (m_view.isMultiviewFBO || m_view.isRecording)
		return FoveationPattern::Left;
	return FoveationPattern::Both;
}

void VulkanRenderer::foveation_uploadPatterns()
{
	if (m_foveation.rateImages.empty())
		return;
	LatteViewFoveationFocus focus;
	const uint32 focusEpoch = LatteView_GetFoveationFocus(focus);
	if (focusEpoch == m_foveation.uploadedEpoch && !m_foveation.hasNewImage)
		return;
	const VkExtent2D texelSize = GetShadingRateTexelSize();
	const uint8 outerRate = m_featureControl.fragmentShadingRate.has4x4 ? RATE_4X4 : RATE_2X2;

	for (auto& rateImage : m_foveation.rateImages)
	{
		if (rateImage->focusEpoch == focusEpoch)
			continue;
		const VkExtent2D extent = rateImage->extent;
		const float aspect = (float)(extent.width * texelSize.width) / (float)(extent.height * texelSize.height);
		auto rateAround = [&](const float centre[2], uint32 x, uint32 y) {
			const float dx = ((x + 0.5f) / extent.width - centre[0]) * aspect;
			const float dy = (y + 0.5f) / extent.height - centre[1];
			const float distance = std::sqrt(dx * dx + dy * dy);
			if (distance < focus.innerRadius)
				return RATE_1X1;
			return distance < focus.outerRadius ? RATE_2X2 : outerRate;
		};
		const uint32 viewLayers = rateImage->viewMask != 0 ? rateImage->layerCount / 2 : 0;
		auto patternOfLayer = [&](uint32 layer) {
			if (rateImage->viewMask == 0)
				return (FoveationPattern)layer;
			if (layer == 0)
				return FoveationPattern::Left;
			return layer == viewLayers - 1 ? FoveationPattern::Right : FoveationPattern::Full;
		};

		const uint32 layerSize = extent.width * extent.height;
		VKRSynchronizedRingAllocator& stagingAllocator = memoryManager->getStagingAllocator();
		auto reservation = stagingAllocator.AllocateBufferMemory(layerSize * rateImage->layerCount, 4);
		for (uint32 layer = 0; layer < rateImage->layerCount; layer++)
		{
			const FoveationPattern pattern = patternOfLayer(layer);
			uint8* rates = reservation.memPtr + layer * layerSize;
			for (uint32 y = 0; y < extent.height; y++)
			{
				for (uint32 x = 0; x < extent.width; x++)
				{
					uint8 rate = RATE_1X1;
					if (pattern == FoveationPattern::Left)
						rate = rateAround(focus.centres[0], x, y);
					else if (pattern == FoveationPattern::Right)
						rate = rateAround(focus.centres[1], x, y);
					else if (pattern == FoveationPattern::Both)
						rate = std::min(rateAround(focus.centres[0], x, y), rateAround(focus.centres[1], x, y));
					rates[y * extent.width + x] = rate;
				}
			}
		}
		stagingAllocator.FlushReservation(reservation);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = rateImage->isInitialized ? VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR : VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = rateImage->image->m_image;
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, rateImage->layerCount };
		vkCmdPipelineBarrier(m_state.currentCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkBufferImageCopy region{};
		region.bufferOffset = reservation.bufferOffset;
		region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, rateImage->layerCount };
		region.imageExtent = { extent.width, extent.height, 1 };
		vkCmdCopyBufferToImage(m_state.currentCommandBuffer, reservation.vkBuffer, rateImage->image->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		vkCmdPipelineBarrier(m_state.currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		rateImage->image->flagForCurrentCommandBuffer();
		rateImage->focusEpoch = focusEpoch;
		rateImage->isInitialized = true;
	}
	m_foveation.uploadedEpoch = focusEpoch;
	m_foveation.hasNewImage = false;
}

void VulkanRenderer::foveation_destroy()
{
	for (auto& rateImage : m_foveation.rateImages)
	{
		for (VKRObjectTextureView* view : rateImage->views)
			ReleaseDestructibleObject(view);
		ReleaseDestructibleObject(rateImage->image);
	}
	m_foveation.rateImages.clear();
}
