#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>
#include <tamashii/renderer_vk/gui_backend.hpp>
#include <rvk/rvk.hpp>

T_BEGIN_NAMESPACE

class VulkanInstance
{
public:
	static constexpr uint32_t FRAMES = 3;
	static constexpr VkFormat COLOR_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
	static constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

	VulkanInstance();
	~VulkanInstance() = default;
	VulkanInstance(const VulkanInstance&) = delete;
	VulkanInstance& operator=(const VulkanInstance&) = delete;
	VulkanInstance(VulkanInstance&&) = delete;
	VulkanInstance& operator=(VulkanInstance&&) = delete;

	void init(tamashii::Window* aWindow);
	void registerWindow(tamashii::Window* aWindow);
	void unregisterWindow(tamashii::Window* aWindow);
	void createFakeSwapchain();
	void destroyFakeSwapchain();
	void printDeviceInfos() const;
	void destroy();

	void setResizeCallback(const std::function<void(uint32_t, uint32_t, rvk::Swapchain&)>& aCallback);
	void recreateSwapchain(uint32_t aWidth, uint32_t aHeight);

	void beginFrame();
	void endFrame();

	GuiBackend* gui() const;

	enum class Vendor { UNDEFINED, NVIDIA, AMD, INTEL };
	Vendor									mVendor;
	std::vector<const char*>				mInstanceExtensions;
	std::vector<const char*>				mValidationLayers;
	std::vector<const char*>				mDeviceExtensions;

	void*									mLib;
	rvk::Instance*							mInstance;
	rvk::LogicalDevice*						mDevice;

	
	std::thread::id							mMainThreadId;
	struct SwapchainFrameData {
		rvk::Semaphore*						mNextImageAvailableSemaphores;	
		rvk::Semaphore*						mRenderFinishedSemaphores;		
		rvk::Fence*							mInFlightFences;
		rvk::CommandBuffer*					mCommandBuffer;
		rvk::Image*							mColor;
		rvk::Image*							mDepth;
	};
	struct SwapchainData {
		tamashii::Window*					mWindow;
		std::optional<rvk::Swapchain>		mSwapchain;
		std::vector<SwapchainFrameData>		mFrames;
		rvk::CommandPool*					mCommandPool; 
		uint32_t							mCurrentImageIndex;
		uint32_t							mPreviousImageIndex;
	};
	SwapchainData							mSwapchainData;
	std::vector<rvk::CommandPool*>			mCommandPools;

	std::function<void(uint32_t, uint32_t, rvk::Swapchain&)> mCallbackSwapchainResize;
	
	VkDebugUtilsMessengerEXT				mDebugCallback;
	VkDebugUtilsMessengerEXT				mDebugCallbackPrintf;
	
	std::unique_ptr<GuiBackend>				mGuiBackend;
private:
	void								setupDebugCallback();
	void								setDefaultState();
};

T_END_NAMESPACE
