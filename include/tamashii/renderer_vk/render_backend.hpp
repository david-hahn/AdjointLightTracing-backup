#pragma once
#include <tamashii/core/scene/render_scene.hpp>
#include <tamashii/core/render/render_backend.hpp>
#include <tamashii/core/gui/main_gui.hpp>
#include "vulkan_instance.hpp"
#include <atomic>

T_BEGIN_NAMESPACE

struct VulkanRenderRoot
{
	rvk::CommandBuffer& currentCmdBuffer() const
	{
		return *instance.mSwapchainData.mFrames[instance.mSwapchainData.mCurrentImageIndex].mCommandBuffer;
	}
	uint32_t currentIndex() const
	{
		return instance.mSwapchainData.mCurrentImageIndex;
	}
	rvk::Image& currentImage() const
	{
		return *instance.mSwapchainData.mFrames[currentIndex()].mColor;
	}
	rvk::Image& currentDepthImage() const
	{
		return *instance.mSwapchainData.mFrames[currentIndex()].mDepth;
	}
	uint32_t previousIndex() const
	{
		return instance.mSwapchainData.mPreviousImageIndex;
	}
	uint32_t frameCount() const
	{
		return static_cast<uint32_t>(instance.mSwapchainData.mFrames.size());
	}
	rvk::SingleTimeCommand singleTimeCommand() const
	{
		if (std::this_thread::get_id() == instance.mMainThreadId) return { instance.mCommandPools[0], instance.mDevice->getQueue(0, 0) };
		return { instance.mCommandPools[1], instance.mDevice->getQueue(0, 1) };
	}
	rvk::LogicalDevice& device;
	VulkanInstance& instance;
};

class VulkanRenderBackend : public RenderBackend {
public:
										VulkanRenderBackend();
										~VulkanRenderBackend() override;

	const char*							getName() override { return "vulkan"; }
	virtual std::vector<RenderBackendImplementation*> initImplementations(VulkanInstance* aInstance) { return {}; }

										
										
    void								init(Window* aMainWindow = nullptr) override;
	void								registerMainWindow(Window* aMainWindow, bool aSizeChanged = true) override;
	void								unregisterMainWindow(Window* aMainWindow) override;
	void								shutdown() override;

	void								addImplementation(RenderBackendImplementation* aImplementation) override;

	void								reloadImplementation(SceneBackendData aScene) override;
	void								changeImplementation(uint32_t aIndex, SceneBackendData aScene) override;
	std::vector<RenderBackendImplementation*>& getAvailableBackendImplementations() override;
	RenderBackendImplementation*		getCurrentBackendImplementations() const override;
										
	void								recreateRenderSurface(uint32_t aWidth, uint32_t aHeight) override;
	void								entitiyAdded(const Ref& aRef) const override;
	void								entitiyRemoved(const Ref& aRef) const override;
	bool								drawOnMesh(const DrawInfo* aDrawInfo) const override;
	void								screenshot(const std::string& aName) const override;

										
										
	void								sceneLoad(SceneBackendData aScene) override;
	void								sceneUnload(SceneBackendData aScene) override;

										
	void								beginFrame() override;
    void								drawView(ViewDef_s* aViewDef) override;
    void								drawUI(UiConf_s* aUiConf) override;
    void								captureSwapchain(ScreenshotInfo_s* aScreenshotInfo) override;
	void								endFrame() override;
private:
										
	void								prepare() const override;
	void								destroy() const override;

	VulkanInstance						mInstance;

    std::atomic<bool>                   mSceneLoaded;
    std::atomic<bool>                   mCmdRecording;

    int									mCurrentImplementationIndex = -1;
    std::vector<RenderBackendImplementation*>mImplementations;

	Window*								mMainWindow;
;	MainGUI								mMainGui;
};

T_END_NAMESPACE
