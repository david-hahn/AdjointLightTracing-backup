#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/image.hpp>

RVK_BEGIN_NAMESPACE
class Semaphore;
class Fence;
class Renderpass;
class Framebuffer;
class CommandBuffer;
class LogicalDevice;
class Swapchain {
public:
													Swapchain(LogicalDevice* aDevice, void* aWindowHandle);
													~Swapchain();

	const VkSurfaceCapabilitiesKHR&					getSurfaceCapabilities() const;
	const std::vector<VkSurfaceFormatKHR>&			getSurfaceFormats() const;
	const std::vector<VkPresentModeKHR>&			getSurfacePresentModes() const;
	std::vector<VkFormat>							getDepthFormats() const;
	bool											checkPresentMode(VkPresentModeKHR aPresentMode) const;
	bool											checkSurfaceFormat(VkSurfaceFormatKHR aSurfaceFormat) const;
	bool											checkDepthFormat(VkFormat aFormat) const;

	bool											setSurfaceFormat(VkSurfaceFormatKHR aSurfaceFormat);
	bool											setPresentMode(VkPresentModeKHR aPresentMode);
	bool											setImageCount(uint32_t aCount);
	bool											useDepth(VkFormat aFormat);

	uint32_t										getImageCount() const;
	VkSurfaceFormatKHR								getSurfaceFormat() const;
	VkFormat										getDepthFormat() const;
	VkExtent2D										getExtent() const;
	VkSwapchainKHR									getHandle() const;

	void											finish();
	void											recreateSwapchain(VkExtent2D aSize = {0, 0});
	void											destroy();
	void											destroyImages();

	uint32_t										getPreviousIndex() const;
	Image*											getPreviousImage();
	Image*											getPreviousDepthImage();
	uint32_t										getCurrentIndex() const;
	Image*											getCurrentImage();
	Image*											getCurrentDepthImage();
	Framebuffer*									getCurrentFramebuffer();
	uint32_t										acquireNextImage(Semaphore *aSemaphore = nullptr, Fence *aFence = nullptr, uint64_t aTimeout = UINT64_MAX);

	std::vector<Image>&								getImages();
	std::vector<Image>&								getDepthImages();
	Renderpass*										getRenderpassClear() const;
	Renderpass*										getRenderpassKeep() const;
	LogicalDevice*									getDevice() const;
	void*											getWindowHandle() const;

	void											CMD_BeginRenderClear(const CommandBuffer* cb, std::vector<VkClearValue> cv);
	void											CMD_BeginRenderKeep(CommandBuffer* cb);
	void											CMD_EndRender(const CommandBuffer* cb);
private:
	friend class									LogicalDevice;

	void											createRenderpasses();
	void											createSwapChain();

	LogicalDevice*									mDevice;

	void*											mWindowHandle;
	VkSurfaceKHR									mSurface;

	VkExtent2D										mExtent;
	VkSurfaceFormatKHR								mSurfaceFormat;
	VkFormat										mDepthFormat;
	VkPresentModeKHR								mPresentMode;
	uint32_t										mImageCount;

	VkSwapchainKHR									mSwapchain;
	Renderpass*										mRpClear;
	Renderpass*										mRpKeep;
	uint32_t										mPreviousIndex;
	uint32_t										mCurrentIndex;
	std::vector<Image>								mImages;
	std::vector<Image>								mDepthImages;
	std::vector<Framebuffer>						mFramebuffers;
};
RVK_END_NAMESPACE
