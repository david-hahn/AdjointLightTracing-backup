#include <rvk/rvk.hpp>
#include <rvk/parts/command_buffer.hpp>
#include "parts/rvk_private.hpp"
RVK_USE_NAMESPACE



std::vector<uint8_t> rvk::swapchain::readPixelsScreen(SingleTimeCommand* aStc, LogicalDevice* aDevice, Image* aSrcImage, bool aAlpha) {
	aDevice->waitIdle();

	const VkExtent3D extent = aSrcImage->getExtent();
	std::vector<uint8_t> buffer;
	buffer.resize(extent.width * extent.height * (aAlpha ? 4 : 3));

	
	VkImageCreateInfo desc{};
	desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	desc.pNext = VK_NULL_HANDLE;
	desc.flags = 0;
	desc.imageType = VK_IMAGE_TYPE_2D;
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.extent.width = extent.width;
	desc.extent.height = extent.height;
	desc.extent.depth = 1;
	desc.mipLevels = 1;
	desc.arrayLayers = 1;
	desc.samples = VK_SAMPLE_COUNT_1_BIT;
	desc.tiling = VK_IMAGE_TILING_LINEAR;
	desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = VK_NULL_HANDLE;
	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage image;
	VK_CHECK(aDevice->vk.CreateImage(aDevice->getHandle(), &desc, VK_NULL_HANDLE, &image), "failed to create image");

	VkMemoryRequirements mRequirements;
	aDevice->vk.GetImageMemoryRequirements(aDevice->getHandle(), image, &mRequirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = VK_NULL_HANDLE;
	alloc_info.allocationSize = mRequirements.size;
	alloc_info.memoryTypeIndex = aDevice->findMemoryTypeIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mRequirements.memoryTypeBits);
	ASSERT(alloc_info.memoryTypeIndex != -1, "could not find suitable Memory");

	VkDeviceMemory memory;
	VK_CHECK(aDevice->vk.AllocateMemory(aDevice->getHandle(), &alloc_info, VK_NULL_HANDLE, &memory), "could not allocate Image Memory");
	VK_CHECK(aDevice->vk.BindImageMemory(aDevice->getHandle(), image, memory, 0), "could not bind Image Memory");

	aStc->begin();

	
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.image = image;

	aDevice->vkCmd.PipelineBarrier(aStc->buffer()->getHandle(),
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE,
		1, &barrier);

	
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;

	VkImageLayout originalLayout = aSrcImage->getLayout();
	aSrcImage->CMD_TransitionImage(aStc->buffer(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VkImageCopy region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel = 0;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource = region.srcSubresource;
	region.dstOffset = region.srcOffset;
	region.extent.width = extent.width;
	region.extent.height = extent.height;
	region.extent.depth = 1;

	aDevice->vkCmd.CopyImage(aStc->buffer()->getHandle(), aSrcImage->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		image, VK_IMAGE_LAYOUT_GENERAL, 1, &region);

	aSrcImage->CMD_TransitionImage(aStc->buffer(), originalLayout);
	aStc->end();

	
	VkImageSubresource subresource{};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;
	VkSubresourceLayout layout = {};
	aDevice->vk.GetImageSubresourceLayout(aDevice->getHandle(), image, &subresource, &layout);

	VkFormat format = aSrcImage->getFormat();
	bool swizzleComponents = (format == VK_FORMAT_B8G8R8A8_SRGB || format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SNORM);

	uint8_t* data;
	VK_CHECK(aDevice->vk.MapMemory(aDevice->getHandle(), memory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&data)), "failed to map memory");

	uint8_t* pBuffer = buffer.data();
	for (uint32_t y = 0; y < extent.height; y++) {
		for (uint32_t x = 0; x < extent.width; x++) {
			uint8_t pixel[4];
			memcpy(&pixel, &data[x * 4], 4);

			
			pBuffer[0] = pixel[0];
			pBuffer[1] = pixel[1];
			pBuffer[2] = pixel[2];
			if (aAlpha) pBuffer[3] = pixel[3];
			if (swizzleComponents) {
				uint8_t temp = pBuffer[0];
				pBuffer[0] = pixel[2];
				pBuffer[2] = temp;
			}

			
			pBuffer += aAlpha ? 4 : 3;
		}
		data += layout.rowPitch;
	}

	aDevice->vk.DestroyImage(aDevice->getHandle(), image, VK_NULL_HANDLE);
	aDevice->vk.FreeMemory(aDevice->getHandle(), memory, VK_NULL_HANDLE);
	return buffer;
}
