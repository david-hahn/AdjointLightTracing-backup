#include "parts/rvk_private.hpp"
#include <rvk/parts/command_buffer.hpp>
RVK_USE_NAMESPACE
class Swapchain;
class CommandBuffer;
using namespace rvk::memory;












/*
** CLEAR ATTACHMENT
*/















































/*
** FULLSCREEN RECT
*/
















































/*
** PIPELINE LIST
*/


































































































































VkPipelineStageFlags rvk::memory::getSrcStageMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout) {
	switch (aOldImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		switch (aNewImageLayout)
		{
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
			return VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
		default:
			return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	default:
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
}
VkPipelineStageFlags rvk::memory::getDstStageMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout) {
	switch (aNewImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	default:
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
}



VkAccessFlags rvk::memory::getSrcAccessMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout) {
	
	
	
	switch (aOldImageLayout)
	{
	
	case VK_IMAGE_LAYOUT_UNDEFINED:
		
		
		
		
		
		if (aNewImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) return VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		return 0;

	
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		
		
		
		return VK_ACCESS_HOST_WRITE_BIT;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		
		
		return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		
		
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		
		
		return VK_ACCESS_TRANSFER_READ_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		
		
		return VK_ACCESS_TRANSFER_WRITE_BIT;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		
		
		return VK_ACCESS_SHADER_READ_BIT;

	default:
		
		return 0;
	}
}
VkAccessFlags rvk::memory::getDstAccessMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout) {
	
			
	switch (aNewImageLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		
		
		return VK_ACCESS_TRANSFER_WRITE_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		
		
		return VK_ACCESS_TRANSFER_READ_BIT;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		
		
		return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		
		
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		
		
		return VK_ACCESS_SHADER_READ_BIT;

	default:
		
		return 0;
	}
}


/*
* Swapchain
*/


































void rvk::swapchain::CMD_CopyImageToCurrentSwapchainImage(const VkCommandBuffer cb, rvk::Image *src_image) {

	

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

	
	
	
	
	
	
	
	
	
	
	

	
	
	
	
	
	
	
	

	
	
	
	
	
	
	
	
	
	
	

	
	
	
	
	
	
	
	
	
	
	
}

void rvk::image::CMD_CopyImageToImage(CommandBuffer* cb, rvk::Image* src_image, rvk::Image* dst_image) {
	LogicalDevice* mDevice = cb->getDevice();
	const VkImageLayout oldSrcLayout = src_image->getLayout();
	const VkImageLayout oldDstLayout = dst_image->getLayout();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	
	
	barrier.oldLayout = oldDstLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	barrier.image = dst_image->getHandle();
	mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, NULL, 0, NULL,
		1, &barrier);

	
	barrier.oldLayout = oldSrcLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(oldSrcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(oldSrcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	barrier.image = src_image->getHandle();
	mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	0, 0, NULL, 0, NULL,
	1, &barrier);

	
	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { dst_image->getExtent().width, dst_image->getExtent().height, 1 };
	mDevice->vkCmd.CopyImage(cb->getHandle(), src_image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = oldDstLayout;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	barrier.image = dst_image->getHandle();
	mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, NULL, 0, NULL,
		1, &barrier);

	
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = oldSrcLayout;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, oldSrcLayout);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, oldSrcLayout);
	barrier.image = src_image->getHandle(); 
	mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, NULL, 0, NULL,
		1, &barrier);
}

void swapchain::CMD_BlitImageToImage(CommandBuffer* cb, rvk::Image* aSrcImage, rvk::Image* aDstImage, VkFilter aFilter)
{
	const LogicalDevice* mDevice = cb->getDevice();
	const VkExtent3D srcExtent = aSrcImage->getExtent();
	const VkImageLayout oldSrcLayout = aSrcImage->getLayout();
	const VkImageLayout oldDstLayout = aDstImage->getLayout();

	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;

	
	barrier.oldLayout = oldDstLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(oldDstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(oldDstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	barrier.image = aDstImage->getHandle();
	mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE,
		1, &barrier);

	
	barrier.oldLayout = oldSrcLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(oldSrcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(oldSrcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	barrier.image = aSrcImage->getHandle();
	if (oldSrcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE,
			1, &barrier);
	}

	
	const VkExtent3D dstExtent = aDstImage->getExtent();
	VkImageBlit blit {};
	blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = { static_cast<int>(dstExtent.width), static_cast<int>(dstExtent.height), 1 };
	blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = { static_cast<int>(srcExtent.width), static_cast<int>(srcExtent.height), 1 };
	mDevice->vkCmd.BlitImage(cb->getHandle(), aSrcImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, aDstImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, aFilter);

	
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = oldDstLayout;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, oldDstLayout);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, oldDstLayout);
	barrier.image = aDstImage->getHandle();
	mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE,
		1, &barrier);

	
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = oldSrcLayout;
	barrier.srcAccessMask = rvk::memory::getSrcAccessMask(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, oldSrcLayout);
	barrier.dstAccessMask = rvk::memory::getDstAccessMask(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, oldSrcLayout);
	barrier.image = aSrcImage->getHandle();
	if (oldSrcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		mDevice->vkCmd.PipelineBarrier(cb->getHandle(),
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE,
			1, &barrier);
	}
}





































































































































/*
* PERFORMANCE
*/






























































const char* rvk::error::errorString(const VkResult aErrorCode)
{
	switch (aErrorCode)
	{
#define STR(r) case VK_ ##r: return #r
        STR(SUCCESS);
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_FRAGMENTED_POOL);
        STR(ERROR_OUT_OF_POOL_MEMORY);
        STR(ERROR_INVALID_EXTERNAL_HANDLE);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
        STR(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
        STR(ERROR_FRAGMENTATION_EXT);
        STR(ERROR_NOT_PERMITTED_EXT);
        STR(ERROR_INVALID_DEVICE_ADDRESS_EXT);
        STR(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
#undef STR
	default:
		return "UNKNOWN_ERROR";
	}
}
