#include <rvk/parts/swapchain.hpp>
#include <rvk/parts/instance.hpp>
#include <rvk/parts/command_buffer.hpp>
#include <rvk/parts/semaphore.hpp>
#include <rvk/parts/fence.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

Swapchain::Swapchain(LogicalDevice* aDevice, void* aWindowHandle) : mDevice{ aDevice }, mWindowHandle{ aWindowHandle },
mExtent{ 0, 0 }, mSurfaceFormat{ }, mDepthFormat{ VK_FORMAT_UNDEFINED },
mPresentMode{ VK_PRESENT_MODE_IMMEDIATE_KHR }, mImageCount{ 0 }, mSwapchain{ VK_NULL_HANDLE }, mRpClear{ nullptr }, mRpKeep{ nullptr }, mPreviousIndex{ static_cast<uint32_t>(-1) }, mCurrentIndex{ static_cast<uint32_t>(-1) }
{
	mSurface = aDevice->getPhysicalDevice()->getInstance()->getSurface(aWindowHandle).mHandle;
	mExtent = getSurfaceCapabilities().currentExtent;
}

Swapchain::~Swapchain()
{
	destroy();
}

const VkSurfaceCapabilitiesKHR& Swapchain::getSurfaceCapabilities() const
{
	return mDevice->getPhysicalDevice()->getSurfaceCapabilities(mWindowHandle);
}

const std::vector<VkSurfaceFormatKHR>& Swapchain::getSurfaceFormats() const
{
	return mDevice->getPhysicalDevice()->getSurfaceFormats(mWindowHandle);
}

const std::vector<VkPresentModeKHR>& Swapchain::getSurfacePresentModes() const
{
	return mDevice->getPhysicalDevice()->getSurfacePresentModes(mWindowHandle);
}

std::vector<VkFormat> Swapchain::getDepthFormats() const
{
	const std::vector<VkFormat> all_formats = {	VK_FORMAT_D16_UNORM,
										VK_FORMAT_X8_D24_UNORM_PACK32,
										VK_FORMAT_D32_SFLOAT,
										VK_FORMAT_S8_UINT,
										VK_FORMAT_D16_UNORM_S8_UINT,
										VK_FORMAT_D24_UNORM_S8_UINT,
										VK_FORMAT_D32_SFLOAT_S8_UINT
									};
	std::vector<VkFormat> supported;
	supported.reserve(all_formats.size());
	for (VkFormat format : all_formats) {
		if(mDevice->getPhysicalDevice()->isFormatSupported(format, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			supported.push_back(format);
		}
	}
	return supported;
}

bool Swapchain::checkPresentMode(const VkPresentModeKHR aPresentMode) const
{
	for(const VkPresentModeKHR e : mDevice->getPhysicalDevice()->getSurfacePresentModes(mWindowHandle))
	{
		if (e == aPresentMode) return true;
	}
	return false;
}

bool Swapchain::checkSurfaceFormat(const VkSurfaceFormatKHR aSurfaceFormat) const
{
	for (const auto [format, colorSpace] : mDevice->getPhysicalDevice()->getSurfaceFormats(mWindowHandle))
	{
		if (aSurfaceFormat.format == format && aSurfaceFormat.colorSpace == colorSpace) return true;
	}
	return false;
}

bool Swapchain::checkDepthFormat(const VkFormat aFormat) const
{
	return mDevice->getPhysicalDevice()->isFormatSupported(aFormat, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool Swapchain::setSurfaceFormat(const VkSurfaceFormatKHR aSurfaceFormat)
{
	bool valid = false;
	if ((valid = checkSurfaceFormat(aSurfaceFormat))) mSurfaceFormat = aSurfaceFormat;
	return valid;
}

bool Swapchain::setPresentMode(const VkPresentModeKHR aPresentMode)
{
	bool valid = false;
	if ((valid = checkPresentMode(aPresentMode))) mPresentMode = aPresentMode;
	return valid;
}

bool Swapchain::setImageCount(const uint32_t aCount)
{
	const VkSurfaceCapabilitiesKHR& sc = getSurfaceCapabilities();
	const uint32_t min = sc.minImageCount;
	const uint32_t max = sc.maxImageCount;
	const bool valid = aCount >= min && aCount <= max;
	if (valid) mImageCount = aCount;
	return valid;
}

bool Swapchain::useDepth(const VkFormat aFormat)
{
	bool valid = false;
	if ((valid = checkDepthFormat(aFormat))) mDepthFormat = aFormat;
	return valid;
}

uint32_t Swapchain::getImageCount() const
{
	return mImageCount;
}

VkSurfaceFormatKHR Swapchain::getSurfaceFormat() const
{
	return mSurfaceFormat;
}

VkFormat Swapchain::getDepthFormat() const
{
	return mDepthFormat;
}

VkExtent2D Swapchain::getExtent() const
{
	return mExtent;
}

VkSwapchainKHR Swapchain::getHandle() const
{
	return mSwapchain;
}

void Swapchain::finish()
{
	createRenderpasses();
	createSwapChain();
}

void Swapchain::recreateSwapchain(const VkExtent2D aSize)
{
	if (aSize.width > 0 && aSize.height > 0) mExtent = aSize;
	else mExtent = getSurfaceCapabilities().currentExtent;
	destroyImages();
	createSwapChain();
}

void Swapchain::destroy()
{
	mDevice->vk.DestroySwapchainKHR(mDevice->getHandle(), mSwapchain, VK_NULL_HANDLE);

	if (mRpClear) {
		delete mRpClear;
		mRpClear = nullptr;
	}
	if (mRpKeep) {
		delete mRpKeep;
		mRpKeep = nullptr;
	}
	destroyImages();
}

void Swapchain::destroyImages()
{
	for (auto& f : mFramebuffers) {
		if (f.mFramebuffer) {
			mDevice->vk.DestroyFramebuffer(mDevice->getHandle(), f.mFramebuffer, nullptr);
			f.mFramebuffer = nullptr;
		}
	}
	mFramebuffers.clear();

	for (auto& i : mImages) {
		i.mImage = VK_NULL_HANDLE;
		if (i.mImageView != VK_NULL_HANDLE) {
			mDevice->vk.DestroyImageView(mDevice->getHandle(), i.mImageView, VK_NULL_HANDLE);
			i.mImageView = VK_NULL_HANDLE;
		}
	}
	mImages.clear();

	for (auto& i : mDepthImages) {
		if (i.mImage != VK_NULL_HANDLE) {
			mDevice->vk.DestroyImage(mDevice->getHandle(), i.mImage, VK_NULL_HANDLE);
			i.mImage = VK_NULL_HANDLE;
		}
		if (i.mImageView != VK_NULL_HANDLE) {
			mDevice->vk.DestroyImageView(mDevice->getHandle(), i.mImageView, VK_NULL_HANDLE);
			i.mImageView = VK_NULL_HANDLE;
		}
		if (i.mMemory != VK_NULL_HANDLE) {
			mDevice->vk.FreeMemory(mDevice->getHandle(), i.mMemory, VK_NULL_HANDLE);
			i.mMemory = VK_NULL_HANDLE;
		}
	}
	mDepthImages.clear();
}

uint32_t Swapchain::getPreviousIndex() const
{
	return mPreviousIndex == static_cast<uint32_t>(-1) ? 0 : mPreviousIndex;
}

Image* Swapchain::getPreviousImage()
{
	return &mImages[getPreviousIndex()];
}

Image* Swapchain::getPreviousDepthImage()
{
	return &mDepthImages[getPreviousIndex()];
}

uint32_t Swapchain::getCurrentIndex() const
{
	return mCurrentIndex;
}

Image* Swapchain::getCurrentImage()
{
	return &mImages[mCurrentIndex == static_cast<uint32_t>(-1) ? 0 : mCurrentIndex];
}

Image* Swapchain::getCurrentDepthImage()
{
	return &mDepthImages[mCurrentIndex == static_cast<uint32_t>(-1) ? 0 : mCurrentIndex];
}

Framebuffer* Swapchain::getCurrentFramebuffer()
{
	return &mFramebuffers[mCurrentIndex == static_cast<uint32_t>(-1) ? 0 : mCurrentIndex];
}

uint32_t Swapchain::acquireNextImage(Semaphore* aSemaphore, Fence* aFence, const uint64_t aTimeout)
{
	mPreviousIndex = mCurrentIndex;
	VK_CHECK(mDevice->vk.AcquireNextImageKHR(mDevice->getHandle(), mSwapchain, aTimeout,
		aSemaphore ? aSemaphore->getHandle() : VK_NULL_HANDLE, aFence ? aFence->getHandle() : VK_NULL_HANDLE, &mCurrentIndex), "failed to acquire next image");
	return mCurrentIndex;
}

std::vector<Image>& Swapchain::getImages()
{
	return mImages;
}

std::vector<Image>& Swapchain::getDepthImages()
{
	return mDepthImages;
}

Renderpass* Swapchain::getRenderpassClear() const
{
	return mRpClear;
}

Renderpass* Swapchain::getRenderpassKeep() const
{
	return mRpKeep;
}

LogicalDevice* Swapchain::getDevice() const
{
	return mDevice;
}

void* Swapchain::getWindowHandle() const
{
	return mWindowHandle;
}

void Swapchain::CMD_BeginRenderClear(const CommandBuffer* cb, const std::vector<VkClearValue> cv)
{
	getCurrentFramebuffer()->setRenderpass(mRpClear);

	VkRenderPassBeginInfo rpBeginInfo = {};
	rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBeginInfo.renderPass = getCurrentFramebuffer()->getRenderpass()->getHandle();
	rpBeginInfo.framebuffer = getCurrentFramebuffer()->getHandle();
	rpBeginInfo.renderArea.extent.width = mExtent.width;
	rpBeginInfo.renderArea.extent.height = mExtent.height;
	rpBeginInfo.clearValueCount = static_cast<uint32_t>(cv.size());
	rpBeginInfo.pClearValues = cv.empty() ? nullptr : cv.data();
	mDevice->vkCmd.BeginRenderPass(cb->getHandle(), &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Swapchain::CMD_BeginRenderKeep(CommandBuffer* cb)
{
	getCurrentFramebuffer()->setRenderpass(mRpKeep);

	VkRenderPassBeginInfo rpBeginInfo = {};
	rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBeginInfo.renderPass = getCurrentFramebuffer()->getRenderpass()->getHandle();
	rpBeginInfo.framebuffer = getCurrentFramebuffer()->getHandle();
	rpBeginInfo.renderArea.extent.width = mExtent.width;
	rpBeginInfo.renderArea.extent.height = mExtent.height;
	mDevice->vkCmd.BeginRenderPass(cb->getHandle(), &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Swapchain::CMD_EndRender(const CommandBuffer* cb)
{
	const Framebuffer* f = getCurrentFramebuffer();
	for (size_t i = 0; i < f->mAttachments.size(); i++)
	{
		f->mAttachments[i]->mLayout = getCurrentFramebuffer()->getRenderpass()->mAttachments[i].finalLayout;
	}
	mDevice->vkCmd.EndRenderPass(cb->getHandle());
}

void Swapchain::createRenderpasses()
{
	mRpClear = new Renderpass(mDevice);
	mRpKeep = new Renderpass(mDevice);
	rvk::AttachmentDescription ca(mSurfaceFormat.format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	ca.setAttachmentOps(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	rvk::AttachmentDescription da(mDepthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	da.setAttachmentOps(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	mRpClear->addAttachment(ca);
	if (mDepthFormat != 0) mRpClear->addAttachment(da);
	mRpClear->addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, 1);
	mRpClear->setColorAttachment(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mRpClear->setDepthStencilAttachment(0, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	mRpClear->finish();

	ca.setAttachmentOps(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
	da.setAttachmentOps(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
	mRpKeep->addAttachment(ca);
	if (mDepthFormat != 0) mRpKeep->addAttachment(da);
	mRpKeep->addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, 1);
	mRpKeep->setColorAttachment(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	mRpKeep->setDepthStencilAttachment(0, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	mRpKeep->finish();
}

void Swapchain::createSwapChain()
{
	VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;
	createInfo.minImageCount = mImageCount;
	createInfo.imageFormat = mSurfaceFormat.format;
	createInfo.imageColorSpace = mSurfaceFormat.colorSpace;
	createInfo.imageExtent = mExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = usage;

	
	
	
	
	
	
	
	
	
	
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	

	createInfo.preTransform = getSurfaceCapabilities().currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = mPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = mSwapchain;

	VK_CHECK(mDevice->vk.CreateSwapchainKHR(mDevice->getHandle(), &createInfo, nullptr, &mSwapchain), "failed to create Swapchain!");

	
	mDevice->vk.GetSwapchainImagesKHR(mDevice->getHandle(), mSwapchain, &mImageCount, nullptr);
	std::vector<VkImage> imgs(mImageCount);
	mDevice->vk.GetSwapchainImagesKHR(mDevice->getHandle(), mSwapchain, &mImageCount, imgs.data());

	mImages.resize(mImageCount, mDevice);
	if (mDepthFormat != 0) mDepthImages.resize(mImageCount, mDevice);
	for (uint32_t i = 0; i < mImageCount; i++)
	{
		{
			mImages[i].mImage = imgs[i];
			mImages[i].mFormat = mSurfaceFormat.format;
			mImages[i].mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			mImages[i].mAspect = VK_IMAGE_ASPECT_COLOR_BIT;
			mImages[i].mUsageFlags = usage;
			mImages[i].mMipLevels = 1;
			mImages[i].mArrayLayers = 1;
			mImages[i].mExtent.width = mExtent.width;
			mImages[i].mExtent.height = mExtent.height;
			mImages[i].mExtent.depth = 1;

			VkImageViewCreateInfo createInfo {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = mImages[i].mImage;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = mImages[i].mFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = mImages[i].mAspect;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = mImages[i].mMipLevels;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = mImages[i].mArrayLayers;
			VK_CHECK(mDevice->vk.CreateImageView(mDevice->getHandle(), &createInfo, NULL, &mImages[i].mImageView), "failed to create ImageView for Swapchain!");
		}
		if (mDepthFormat != 0)
		{
			mDepthImages[i].createImage2D(mExtent.width, mExtent.height, mDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			/*depth_images[i].mFormat = depth_format;
			depth_images[i].mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			switch (depth_format) {
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				depth_images[i].mAspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT; break;
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_S8_UINT:
			default:
				depth_images[i].mAspect = VK_IMAGE_ASPECT_DEPTH_BIT; break;
			}
			depth_images[i].mMipLevels = 1;
			depth_images[i].mArrayLayers = 1;
			depth_images[i].mExtent.width = extent.width;
			depth_images[i].mExtent.height = extent.height;
			depth_images[i].mExtent.depth = 1;

			VkImageCreateInfo ici = {};
			ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			ici.pNext = VK_NULL_HANDLE;
			ici.flags = 0;
			ici.imageType = VK_IMAGE_TYPE_2D;
			ici.format = depth_format;
			ici.extent.width = extent.width;
			ici.extent.height = extent.height;
			ici.extent.depth = 1;
			ici.mipLevels = 1;
			ici.arrayLayers = 1;
			ici.samples = VK_SAMPLE_COUNT_1_BIT;
			ici.tiling = VK_IMAGE_TILING_OPTIMAL;
			ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			ici.queueFamilyIndexCount = 0;
			ici.pQueueFamilyIndices = VK_NULL_HANDLE;
			ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VK_CHECK(device->vk.CreateImage(device->getHandle(), &ici, VK_NULL_HANDLE, &depth_images[i].mImage), "failed to create Image for Swapchain!")

			VkMemoryRequirements memReq = device->getImageMemoryRequirements(depth_images[i].mImage);
			const int memory_index = device->getPhysicalDevice()->findMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memReq.memoryTypeBits);
			ASSERT(memory_index != -1, "could not find suitable Memory");
			depth_images[i].mMemory = device->allocMemory(memReq.size, static_cast<uint32_t>(memory_index));
			VK_CHECK(device->vk.BindImageMemory(device->getHandle(), depth_images[i].mImage, depth_images[i].mMemory, 0), "failed to bind Image Memory!");

			VkImageViewCreateInfo ivci = {};
			ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ivci.image = depth_images[i].mImage;
			ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ivci.format = depth_images[i].mFormat;
			ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivci.subresourceRange.aspectMask = depth_images[i].mAspect;
			ivci.subresourceRange.baseMipLevel = 0;
			ivci.subresourceRange.levelCount = depth_images[i].mMipLevels;
			ivci.subresourceRange.baseArrayLayer = 0;
			ivci.subresourceRange.layerCount = depth_images[i].mArrayLayers;
			VK_CHECK(device->vk.CreateImageView(device->getHandle(), &ivci, NULL, &depth_images[i].mImageView), "failed to create ImageView for Swapchain!")*/
		}
	}

	
	mFramebuffers.resize(mImageCount, mDevice);
	for (uint32_t i = 0; i < mImageCount; i++)
	{
		mFramebuffers[i].addAttachment(&mImages[i]);
		if (mDepthFormat != 0) mFramebuffers[i].addAttachment(&mDepthImages[i]);
		mFramebuffers[i].setRenderpass(mRpClear);

		std::vector<VkImageView> views;
		views.reserve(mRpClear->mAttachments.size());
		for (const Image* attachment : mFramebuffers[i].mAttachments) views.push_back(attachment->mImageView);
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRpClear->mRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
		framebufferInfo.pAttachments = toArrayPointer(views);
		framebufferInfo.width = mExtent.width;
		framebufferInfo.height = mExtent.height;
		framebufferInfo.layers = 1;
		VK_CHECK(mDevice->vk.CreateFramebuffer(mDevice->getHandle(), &framebufferInfo, NULL, &mFramebuffers[i].mFramebuffer), "failed to create Swapchain Framebuffer");
	}
}
