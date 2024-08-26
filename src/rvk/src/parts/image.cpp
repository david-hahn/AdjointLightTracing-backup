#include <rvk/parts/image.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

Image::Image(LogicalDevice* aDevice) : mDevice(aDevice), mExtent({ 0, 0, 0 }), mFormat(VK_FORMAT_UNDEFINED), mProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), mAspect(VK_IMAGE_ASPECT_COLOR_BIT), mMipLevels(0),
mArrayLayers(0), mLayout(VK_IMAGE_LAYOUT_UNDEFINED), mUsageFlags(0), mImage(VK_NULL_HANDLE), mImageView(VK_NULL_HANDLE), mSampler(VK_NULL_HANDLE), mMemory(VK_NULL_HANDLE) {}



rvk::Image::~Image() {
	destroy();
}

bool Image::isInit() const
{ return mImage != VK_NULL_HANDLE; }

VkImage Image::getHandle() const
{ return mImage; }

VkImageView Image::getImageView() const
{
	return mImageView;
}

VkSampler Image::getSampler() const
{ return mSampler; }

VkImageLayout Image::getLayout() const
{ return mLayout; }

VkFormat Image::getFormat() const
{ return mFormat; }

VkExtent3D Image::getExtent() const
{ return mExtent; }

uint32_t Image::getArrayLayerCount() const
{ return mArrayLayers; }

uint32_t Image::getMipMapCount() const
{ return mMipLevels; }

void rvk::Image::createImage2D(const uint32_t aWidth, const uint32_t aHeight, const VkFormat aFormat, 
                               const VkImageUsageFlags aUsage, const uint32_t aMipLevels, const uint32_t aArrayLayers) {
	mExtent = { aWidth, aHeight, 1 };
	mFormat = aFormat;
	mMipLevels = aMipLevels;
	mArrayLayers = aArrayLayers;
	mUsageFlags = aUsage;
	mProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	createImage(VK_IMAGE_TYPE_2D, aUsage);
	if(aArrayLayers == 1)createImageView(VK_IMAGE_VIEW_TYPE_2D);
	else if (aArrayLayers > 1)createImageView(VK_IMAGE_VIEW_TYPE_2D_ARRAY);
}

void Image::createCubeImage(uint32_t aWidth, uint32_t aHeight, VkFormat aFormat, VkImageUsageFlags aUsage,
	uint32_t aMipLevels)
{
	mExtent = { aWidth, aHeight, 1 };
	mFormat = aFormat;
	mMipLevels = aMipLevels;
	mArrayLayers = 6;
	mUsageFlags = aUsage;
	mProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	createImage(VK_IMAGE_TYPE_2D, aUsage, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
	createImageView(VK_IMAGE_VIEW_TYPE_CUBE);
}

void rvk::Image::setSampler(const SamplerConfig aSamplerConfig)
{
	mSampler = mDevice->getSampler(aSamplerConfig)->getHandle();
}

void Image::setSampler(const VkSampler aSampler)
{ mSampler = aSampler; }

void Image::CMD_ClearColor(const CommandBuffer* aCmdBuffer, const float aR, const float aG, const float aB, const float aA) const
{
	if (!isBitSet<VkBufferUsageFlags>(mUsageFlags, VK_IMAGE_USAGE_TRANSFER_DST_BIT)) Logger::warning("Image needs VK_IMAGE_USAGE_TRANSFER_DST_BIT to be set to allow clearColor cmd");
	const VkClearColorValue cv = { aR, aG, aB, aA };
	VkImageSubresourceRange sr = {};
	sr.layerCount = mArrayLayers;
	sr.levelCount = mMipLevels;
	sr.aspectMask = mAspect;
	aCmdBuffer->getDevice()->vkCmd.ClearColorImage(aCmdBuffer->getHandle(), mImage, mLayout, &cv, 1, &sr);
}

void Image::CMD_TransitionImage(const CommandBuffer* aCmdBuffer, VkImageLayout aNewLayout)
{
	VkImageMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.subresourceRange.aspectMask = mAspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.levelCount = mMipLevels;
	barrier.subresourceRange.layerCount = mArrayLayers;

	if (aNewLayout == VK_IMAGE_LAYOUT_UNDEFINED) aNewLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcAccessMask = memory::getSrcAccessMask(mLayout, aNewLayout);
	barrier.dstAccessMask = memory::getDstAccessMask(mLayout, aNewLayout);

	barrier.oldLayout = mLayout;
	barrier.newLayout = aNewLayout;
	barrier.image = mImage;

	aCmdBuffer->getDevice()->vkCmd.PipelineBarrier(aCmdBuffer->getHandle(),
		memory::getSrcStageMask(mLayout, aNewLayout),
		memory::getDstStageMask(mLayout, aNewLayout),
		0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);
	mLayout = aNewLayout;
}

void Image::CMD_CopyBufferToImage(const CommandBuffer* aCmdBuffer, const Buffer* aSrcBuffer, const uint32_t aMipLevel,
                                  const uint32_t aArrayLayer, VkExtent3D aImageExtent, const VkOffset3D aImageOffset, const VkDeviceSize aBufferOffset) const
{
	if (aImageExtent.width == 0 && aImageExtent.height == 0 && aImageExtent.depth == 0) aImageExtent = mExtent;

	VkBufferImageCopy region = {};
	region.bufferOffset = aBufferOffset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = mAspect;
	region.imageSubresource.mipLevel = aMipLevel;
	region.imageSubresource.baseArrayLayer = aArrayLayer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = aImageOffset;
	region.imageExtent = aImageExtent;
	mDevice->vkCmd.CopyBufferToImage(aCmdBuffer->getHandle(), aSrcBuffer->getRawHandle(), mImage, mLayout, 1, &region);
}

void Image::CMD_CopyImageToBuffer(const CommandBuffer* aCmdBuffer, const Buffer* aDstBuffer, const uint32_t aMipLevel,
	uint32_t aArrayLayer, VkExtent3D aImageExtent, const VkOffset3D aImageOffset, const VkDeviceSize aBufferOffset) const
{
	if (aImageExtent.width == 0 && aImageExtent.height == 0 && aImageExtent.depth == 0) aImageExtent = mExtent;

	VkBufferImageCopy region = {};
	region.bufferOffset = aBufferOffset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = mAspect;
	region.imageSubresource.mipLevel = aMipLevel;
	region.imageSubresource.baseArrayLayer = aArrayLayer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = aImageOffset;
	region.imageExtent = aImageExtent;
	mDevice->vkCmd.CopyImageToBuffer(aCmdBuffer->getHandle(), mImage, mLayout, aDstBuffer->getRawHandle(), 1, &region);
}


















void Image::CMD_GenerateMipmaps(const CommandBuffer* aCmdBuffer) const
{
	VkImageLayout old_layout = mLayout;
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = mImage;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = mAspect;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mMipLevels;
	barrier.oldLayout = old_layout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = memory::getSrcAccessMask(barrier.oldLayout, barrier.newLayout);
	barrier.dstAccessMask = memory::getDstAccessMask(barrier.oldLayout, barrier.newLayout);
	mDevice->vkCmd.PipelineBarrier(aCmdBuffer->getHandle(),
		memory::getSrcStageMask(barrier.oldLayout, barrier.newLayout),
		memory::getDstStageMask(barrier.oldLayout, barrier.newLayout),
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = mExtent.width;
	int32_t mipHeight = mExtent.height;

	for (uint32_t i = 1; i < mMipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = memory::getSrcAccessMask(barrier.oldLayout, barrier.newLayout);
		barrier.dstAccessMask = memory::getDstAccessMask(barrier.oldLayout, barrier.newLayout);

		mDevice->vkCmd.PipelineBarrier(aCmdBuffer->getHandle(),
			memory::getSrcStageMask(barrier.oldLayout, barrier.newLayout),
			memory::getDstStageMask(barrier.oldLayout, barrier.newLayout),
			0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = mAspect;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		mDevice->vkCmd.BlitImage(aCmdBuffer->getHandle(),
			mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = old_layout;
		barrier.srcAccessMask = memory::getSrcAccessMask(barrier.oldLayout, barrier.newLayout);
		barrier.dstAccessMask = memory::getDstAccessMask(barrier.oldLayout, barrier.newLayout);

		mDevice->vkCmd.PipelineBarrier(aCmdBuffer->getHandle(),
			memory::getSrcStageMask(barrier.oldLayout, barrier.newLayout),
			memory::getDstStageMask(barrier.oldLayout, barrier.newLayout),
			0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mMipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = old_layout;
	barrier.srcAccessMask = memory::getSrcAccessMask(barrier.oldLayout, barrier.newLayout);
	barrier.dstAccessMask = memory::getDstAccessMask(barrier.oldLayout, barrier.newLayout);

	mDevice->vkCmd.PipelineBarrier(aCmdBuffer->getHandle(),
		memory::getSrcStageMask(barrier.oldLayout, barrier.newLayout),
		memory::getDstStageMask(barrier.oldLayout, barrier.newLayout),
		0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Image::CMD_BlitImage(const CommandBuffer* aCmdBuffer, const rvk::Image* aSrcImage, const VkFilter aFilter) const
{
	const VkExtent3D srcExtent = aSrcImage->getExtent();
	const VkExtent3D dstExtent = mExtent;
	VkImageBlit blit = {};
	blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 };
	blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 };
	mDevice->vkCmd.BlitImage(aCmdBuffer->getHandle(), aSrcImage->getHandle(), aSrcImage->getLayout(), mImage, mLayout, 1, &blit, aFilter);
}

void Image::CMD_ImageBarrier(const CommandBuffer* aCmdBuffer, const VkPipelineStageFlags aSrcStage, const VkPipelineStageFlags aDstStage,
                             const VkAccessFlags aSrcAccess, const VkAccessFlags aDstAccess, const VkDependencyFlags aDependency) const
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = mImage;
	barrier.srcAccessMask = aSrcAccess;
	barrier.dstAccessMask = aDstAccess;
	barrier.oldLayout = mLayout;
	barrier.newLayout = mLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = mAspect;
	barrier.subresourceRange.levelCount = mMipLevels;
	barrier.subresourceRange.layerCount = mArrayLayers;
	mDevice->vkCmd.PipelineBarrier(aCmdBuffer->getHandle(),
		aSrcStage,
		aDstStage,
		aDependency,
		0, VK_NULL_HANDLE,
		0, VK_NULL_HANDLE,
		1, &barrier);
}

void Image::STC_UploadData2D(SingleTimeCommand* aStc, const uint32_t aWidth, const uint32_t aHeight, const uint32_t aBytesPerPixel,
                             const void* aDataIn, uint32_t aMipLevel, uint32_t aArrayLayer)
{
	const VkDeviceSize imageSize = static_cast<uint64_t>(aWidth) * static_cast<uint64_t>(aHeight) * static_cast<uint64_t>(1) * static_cast<uint64_t>(aBytesPerPixel);

	Buffer stagingBuffer(mDevice);
	stagingBuffer.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	
	stagingBuffer.mapBuffer();
	uint8_t* p = stagingBuffer.getMemoryPointer();
	std::memcpy(p, aDataIn, (size_t)(imageSize));
	stagingBuffer.unmapBuffer();
	STC_CopyBufferToImage(aStc, &stagingBuffer, aMipLevel, aArrayLayer);
}

void Image::STC_DownloadData2D(SingleTimeCommand* aStc, uint32_t aWidth, uint32_t aHeight, uint32_t aBytesPerPixel,
	void* aDataOut, uint32_t aArrayLayer)
{
	const VkDeviceSize imageSize = static_cast<uint64_t>(aWidth) * static_cast<uint64_t>(aHeight) * static_cast<uint64_t>(1) * static_cast<uint64_t>(aBytesPerPixel);

	Buffer stagingBuffer(mDevice);
	stagingBuffer.create(VK_BUFFER_USAGE_TRANSFER_DST_BIT, imageSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	STC_CopyImageToBuffer(aStc, &stagingBuffer);

	
	stagingBuffer.mapBuffer();
	uint8_t* p = stagingBuffer.getMemoryPointer();
	std::memcpy(aDataOut, p, (size_t)(imageSize));
	stagingBuffer.unmapBuffer();
}

void Image::STC_GenerateMipmaps(SingleTimeCommand* aStc) const
{
	aStc->begin();
	CMD_GenerateMipmaps(aStc->buffer());
	aStc->end();
}

void Image::STC_TransitionImage(SingleTimeCommand* aStc, VkImageLayout aNewLayout)
{
	aStc->begin();
	CMD_TransitionImage(aStc->buffer(), aNewLayout);
	aStc->end();
}

void rvk::Image::destroy()
{
	if (mImage != VK_NULL_HANDLE) {
		mDevice->vk.DestroyImage(mDevice->getHandle(), mImage, VK_NULL_HANDLE);
		mImage = VK_NULL_HANDLE;
	}
	if (mImageView != VK_NULL_HANDLE) {
		mDevice->vk.DestroyImageView(mDevice->getHandle(), mImageView, VK_NULL_HANDLE);
		mImageView = VK_NULL_HANDLE;
	}
	if (mMemory != VK_NULL_HANDLE) {
		mDevice->vk.FreeMemory(mDevice->getHandle(), mMemory, VK_NULL_HANDLE);
		mMemory = VK_NULL_HANDLE;
	}
	mSampler = VK_NULL_HANDLE;
	mExtent = { 0, 0, 0 };
	mFormat = VK_FORMAT_UNDEFINED;
	mAspect = 0;
	mMipLevels = 0;
	mArrayLayers = 0;
	mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	mUsageFlags = 0;
}

/**
* Private 
**/
void rvk::Image::createImage(const VkImageType aImageType, const VkImageUsageFlags aUsage, const VkImageCreateFlags aFlags)
{
	VkImageCreateInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	desc.pNext = VK_NULL_HANDLE;
	desc.flags = aFlags;
	desc.imageType = aImageType;
	desc.format = mFormat;
	desc.extent.width = mExtent.width;
	desc.extent.height = mExtent.height;
	desc.extent.depth = 1;
	desc.mipLevels = mMipLevels;
	desc.arrayLayers = mArrayLayers;
	desc.samples = VK_SAMPLE_COUNT_1_BIT;
	desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	desc.usage = aUsage;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = VK_NULL_HANDLE;
	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CHECK(mDevice->vk.CreateImage(mDevice->getHandle(), &desc, VK_NULL_HANDLE, &mImage), "failed to create Image!");
	createImageMemory();
}
void rvk::Image::createImageView(const VkImageViewType aViewType)
{
	switch (mFormat) {
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		mAspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT; break;
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_D32_SFLOAT:
		mAspect = VK_IMAGE_ASPECT_DEPTH_BIT; break;
	case VK_FORMAT_S8_UINT:
		mAspect = VK_IMAGE_ASPECT_DEPTH_BIT; break;
	default:
		mAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageViewCreateInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	desc.pNext = VK_NULL_HANDLE;
	desc.flags = 0;
	desc.image = mImage;
	desc.viewType = aViewType;
	desc.format = mFormat;
	desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	desc.subresourceRange.aspectMask = mAspect;
	desc.subresourceRange.baseMipLevel = 0;
	desc.subresourceRange.levelCount = mMipLevels;
	desc.subresourceRange.baseArrayLayer = 0;
	desc.subresourceRange.layerCount = mArrayLayers;
	VK_CHECK(mDevice->vk.CreateImageView(mDevice->getHandle(), &desc, VK_NULL_HANDLE, &mImageView), "failed to create Image View!");
}

void Image::STC_CopyBufferToImage(SingleTimeCommand* aStc, const Buffer* aBuffer, uint32_t aMipLevel,
	uint32_t aArrayLayer, VkExtent3D aImageExtent, VkOffset3D aImageOffset, VkDeviceSize aBufferOffset)
{
	const VkImageLayout oldLayout = mLayout;
	aStc->begin();
	CMD_TransitionImage(aStc->buffer(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CMD_CopyBufferToImage(aStc->buffer(), aBuffer, aMipLevel, aArrayLayer, aImageExtent, aImageOffset, aBufferOffset);
	CMD_TransitionImage(aStc->buffer(), oldLayout);
	aStc->end();
}

void Image::STC_CopyImageToBuffer(SingleTimeCommand* aStc, const Buffer* aBuffer, uint32_t aMipLevel,
	uint32_t ArrayLayer, VkExtent3D aImageExtent, VkOffset3D aImageOffset, VkDeviceSize aBufferOffset)
{
	const VkImageLayout oldLayout = mLayout;
	aStc->begin();
	CMD_TransitionImage(aStc->buffer(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	CMD_CopyImageToBuffer(aStc->buffer(), aBuffer, aMipLevel, ArrayLayer, aImageExtent, aImageOffset, aBufferOffset);
	CMD_TransitionImage(aStc->buffer(), oldLayout);
	aStc->end();
}

void Image::createImageMemory() {
	VkMemoryRequirements memRequirements = {};
	mDevice->vk.GetImageMemoryRequirements(mDevice->getHandle(), mImage, &memRequirements);

	const VkMemoryAllocateInfo memAllocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		VK_NULL_HANDLE,
		memRequirements.size,
		static_cast<uint32_t>(mDevice->findMemoryTypeIndex(mProperties, memRequirements.memoryTypeBits))
	};
	ASSERT(memAllocInfo.memoryTypeIndex != -1, "could not find suitable Memory");

	VK_CHECK(mDevice->vk.AllocateMemory(mDevice->getHandle(), &memAllocInfo, NULL, &mMemory), "failed to allocate Image Memory!");
	VK_CHECK(mDevice->vk.BindImageMemory(mDevice->getHandle(), mImage, mMemory, 0), "failed to bind Image Memory!");
}































































































































































































































































































































