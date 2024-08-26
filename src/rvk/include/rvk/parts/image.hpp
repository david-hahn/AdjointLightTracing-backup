#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/sampler_config.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class Buffer;
class CommandBuffer;
class SingleTimeCommand;
class Image {
public:

	
	enum Use {
		
		SAMPLED = VK_IMAGE_USAGE_SAMPLED_BIT,
		
		STORAGE = VK_IMAGE_USAGE_STORAGE_BIT,
		
		UPLOAD = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		DOWNLOAD = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		COLOR_ATTACHMENT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		DEPTH_STENCIL_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
	};

													Image(LogicalDevice* aDevice);
													~Image();

	bool											isInit() const;
	VkImage											getHandle() const;
	VkImageView										getImageView() const;
	VkSampler										getSampler() const;
	VkImageLayout									getLayout() const;
	VkFormat										getFormat() const;
	VkExtent3D										getExtent() const;
	uint32_t										getArrayLayerCount() const;
	uint32_t										getMipMapCount() const;

	void											createImage2D(uint32_t aWidth, uint32_t aHeight, VkFormat aFormat,
	                                                              VkImageUsageFlags aUsage, uint32_t aMipLevels = 1, uint32_t aArrayLayers = 1);
	void											createCubeImage(uint32_t aWidth, uint32_t aHeight, VkFormat aFormat,
														VkImageUsageFlags aUsage, uint32_t aMipLevels = 1);
													
	void											setSampler(SamplerConfig aSamplerConfig);
	void											setSampler(VkSampler aSampler);
	void											destroy();

													
	
													
	void											CMD_ClearColor(const CommandBuffer* aCmdBuffer, float aR, float aG, float aB, float aA) const;
	void											CMD_TransitionImage(const CommandBuffer* aCmdBuffer, VkImageLayout aNewLayout);
	void											CMD_CopyBufferToImage(const CommandBuffer* aCmdBuffer, const Buffer* aSrcBuffer, uint32_t aMipLevel = 0, uint32_t aArrayLayer = 0, VkExtent3D aImageExtent = {}, VkOffset3D aImageOffset = {}, VkDeviceSize aBufferOffset = 0) const;
	void											CMD_CopyImageToBuffer(const CommandBuffer* aCmdBuffer, const Buffer* aDstBuffer, uint32_t aMipLevel = 0, uint32_t aArrayLayer = 0, VkExtent3D aImageExtent = {}, VkOffset3D aImageOffset = {}, VkDeviceSize aBufferOffset = 0) const;
	
	void											CMD_GenerateMipmaps(const CommandBuffer* aCmdBuffer) const;
	void											CMD_BlitImage(const CommandBuffer* aCmdBuffer, const rvk::Image* aSrcImage, VkFilter aFilter) const;
	void											CMD_ImageBarrier(const CommandBuffer* aCmdBuffer, VkPipelineStageFlags aSrcStage, VkPipelineStageFlags aDstStage, VkAccessFlags aSrcAccess, VkAccessFlags aDstAccess, VkDependencyFlags aDependency) const;

													
													
	void											STC_UploadData2D(SingleTimeCommand* aStc, uint32_t aWidth, uint32_t aHeight, uint32_t aBytesPerPixel,
														const void* aDataIn, uint32_t aMipLevel = 0, uint32_t aArrayLayer = 0);
	void											STC_DownloadData2D(SingleTimeCommand* aStc, uint32_t aWidth, uint32_t aHeight, uint32_t aBytesPerPixel,
														void* aDataOut, uint32_t aArrayLayer = 0);
	void											STC_GenerateMipmaps(SingleTimeCommand* aStc) const;
	void											STC_TransitionImage(SingleTimeCommand* aStc, VkImageLayout aNewLayout);
private:
	friend class									Descriptor;
	friend class									Framebuffer;
	friend class									Swapchain;

	void											createImage(VkImageType aImageType, VkImageUsageFlags aUsage, VkImageCreateFlags aFlags = 0);
	void											createImageView(VkImageViewType aViewType);

													
	void											STC_CopyBufferToImage(SingleTimeCommand* aStc, const Buffer* aBuffer, uint32_t aMipLevel = 0, uint32_t aArrayLayer = 0, VkExtent3D aImageExtent = {}, VkOffset3D aImageOffset = {}, VkDeviceSize aBufferOffset = 0);
	void											STC_CopyImageToBuffer(SingleTimeCommand* aStc, const Buffer* aBuffer, uint32_t aMipLevel = 0, uint32_t ArrayLayer = 0, VkExtent3D aImageExtent = {}, VkOffset3D aImageOffset = {}, VkDeviceSize aBufferOffset = 0);


	void											createImageMemory();

	LogicalDevice*									mDevice;

	VkExtent3D										mExtent;
	VkFormat										mFormat;
	VkMemoryPropertyFlags							mProperties;
	VkImageAspectFlags								mAspect;
	uint32_t										mMipLevels;
	uint32_t										mArrayLayers;
	VkImageLayout                                   mLayout;
	VkImageUsageFlags								mUsageFlags;

	VkImage											mImage;
	VkImageView										mImageView;
	VkSampler										mSampler;
	VkDeviceMemory									mMemory;
};
RVK_END_NAMESPACE