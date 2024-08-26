#include <tamashii/renderer_vk/render_backend.hpp>

#include <rvk/rvk.hpp>
#include <rvk/shader_compiler.hpp>

#include <tamashii/core/common/common.hpp>
#include <tamashii/core/platform/filewatcher.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/render/render_backend_implementation.hpp>

#include <tamashii/core/gui/main_gui.hpp>

T_USE_NAMESPACE
RVK_USE_NAMESPACE

VulkanRenderBackend::VulkanRenderBackend() : mSceneLoaded(false), mCmdRecording(false),
	mCurrentImplementationIndex(0), mMainWindow(nullptr)
{
}

VulkanRenderBackend::~VulkanRenderBackend()
{
	for (const RenderBackendImplementation* impl : mImplementations) {
		delete impl;
	}
}

void VulkanRenderBackend::init(Window* aMainWindow) {
	scomp::init(); 
	mInstance.init(aMainWindow);
	mMainWindow = aMainWindow;

	mInstance.setResizeCallback([this](uint32_t aWidth, uint32_t aHeight, rvk::Swapchain& sc) {
        Common::getInstance().getMainWindow()->setSize({aWidth, aHeight});
        rvk::RPipeline::global_render_state.scissor.extent.width = aWidth;
        rvk::RPipeline::global_render_state.scissor.extent.height = aHeight;
        rvk::RPipeline::global_render_state.viewport.width = static_cast<float>(aWidth);
        rvk::RPipeline::global_render_state.viewport.height = static_cast<float>(aHeight);

        if(!mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->windowSizeChanged(aWidth, aHeight);
    });
	spdlog::info("...done");
	mInstance.printDeviceInfos();

	
	rvk::Shader::cache = true;
	rvk::Shader::cache_dir = var::cache_dir.value() + "/shader";
	rvk::Pipeline::pcache = true;
	rvk::Pipeline::cache_dir = var::cache_dir.value();

	
	int major, minor, patch;
	scomp::getGlslangVersion(major, minor, patch);
	spdlog::info("Glslang Version: {}.{}.{}", major, minor, patch);

	
	FileWatcher::getInstance().setInitCallback([]() { scomp::init(); });
	FileWatcher::getInstance().setShutdownCallback([]() { scomp::finalize(); });

	mImplementations = initImplementations(&mInstance);
	for (int i = 0; i < static_cast<int>(mImplementations.size()); i++) {
		if (mImplementations[i]->getName() == var::default_implementation.value()) mCurrentImplementationIndex = i;
	}

	prepare();
}

void VulkanRenderBackend::registerMainWindow(Window* aMainWindow, bool aSizeChanged)
{
	if (mMainWindow == aMainWindow) return;
	if (mMainWindow) unregisterMainWindow(aMainWindow);
	mMainWindow = aMainWindow;
	mInstance.registerWindow(mMainWindow);

	const auto extent = mInstance.mSwapchainData.mSwapchain->getExtent();
	rvk::RPipeline::global_render_state.scissor.extent.width = extent.width;
	rvk::RPipeline::global_render_state.scissor.extent.height = extent.height;
	rvk::RPipeline::global_render_state.viewport.width = static_cast<float>(extent.width);
	rvk::RPipeline::global_render_state.viewport.height = static_cast<float>(extent.height);
	if (!mImplementations.empty() && aSizeChanged) mImplementations[mCurrentImplementationIndex]->windowSizeChanged(extent.width, extent.height);
}

void VulkanRenderBackend::unregisterMainWindow(Window* aMainWindow)
{
	if (!mMainWindow) return;
	mMainWindow = nullptr;
	mInstance.mDevice->waitIdle();
	mInstance.unregisterWindow(aMainWindow);

	const VkExtent2D extent { static_cast<uint32_t>(var::render_size.value().at(0)), static_cast<uint32_t>(var::render_size.value().at(1)) };
	rvk::RPipeline::global_render_state.scissor.extent.width = extent.width;
	rvk::RPipeline::global_render_state.scissor.extent.height = extent.height;
	rvk::RPipeline::global_render_state.viewport.width = static_cast<float>(extent.width);
	rvk::RPipeline::global_render_state.viewport.height = static_cast<float>(extent.height);
	if (!mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->windowSizeChanged(extent.width, extent.height);
}

void VulkanRenderBackend::shutdown()
{
	spdlog::info("...shutting down Vulkan");
	scomp::finalize(); 
	mInstance.mDevice->waitIdle();

	
	destroy();
	mInstance.destroy();
}

void VulkanRenderBackend::addImplementation(RenderBackendImplementation* aImplementation)
{
	if (!mImplementations.empty()) mImplementations.push_back(aImplementation);
}

void VulkanRenderBackend::reloadImplementation(SceneBackendData aScene)
{
	
	mInstance.mDevice->waitIdle();
	sceneUnload(aScene);
	destroy();
	spdlog::info("Reloading backend implementation: {}", mImplementations[mCurrentImplementationIndex]->getName());
	prepare();
	sceneLoad(aScene);
}

void VulkanRenderBackend::changeImplementation(const uint32_t aIndex, const SceneBackendData aScene)
{
	
	mInstance.mDevice->waitIdle();
	sceneUnload(aScene);
	destroy();
	mCurrentImplementationIndex = aIndex;
	spdlog::info("Changing backend implementation to: {}", mImplementations[mCurrentImplementationIndex]->getName());
	prepare();
	sceneLoad(aScene);
}

std::vector<RenderBackendImplementation*>& VulkanRenderBackend::getAvailableBackendImplementations()
{ return mImplementations; }

RenderBackendImplementation* VulkanRenderBackend::getCurrentBackendImplementations() const
{ return !mImplementations.empty() ? mImplementations[mCurrentImplementationIndex] : nullptr; }

void VulkanRenderBackend::recreateRenderSurface(const uint32_t aWidth, const uint32_t aHeight)
{
	mInstance.recreateSwapchain(aWidth, aHeight);
	const VkExtent2D extent = mInstance.mSwapchainData.mSwapchain->getExtent();

    rvk::RPipeline::global_render_state.scissor.extent.width = extent.width;
    rvk::RPipeline::global_render_state.scissor.extent.height = extent.height;
	rvk::RPipeline::global_render_state.viewport.width = static_cast<float>(extent.width);
	rvk::RPipeline::global_render_state.viewport.height = static_cast<float>(extent.height);

	if (!mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->windowSizeChanged(extent.width, extent.height);
}

void VulkanRenderBackend::entitiyAdded(const Ref& aRef) const
{
	if (!mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->entityAdded(aRef);
}

void VulkanRenderBackend::entitiyRemoved(const Ref& aRef) const
{
	if (!mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->entityRemoved(aRef);
}

bool VulkanRenderBackend::drawOnMesh(const DrawInfo* aDrawInfo) const
{
	if (mImplementations.empty()) return true;
	return mImplementations[mCurrentImplementationIndex]->drawOnMesh(aDrawInfo);
}

void VulkanRenderBackend::screenshot(const std::string& aName) const
{
	if (mImplementations.empty()) return;
	return mImplementations[mCurrentImplementationIndex]->screenshot(aName);
}

void VulkanRenderBackend::sceneLoad(const SceneBackendData aScene) {
	if (!mImplementations.empty()) {
		mImplementations[mCurrentImplementationIndex]->sceneLoad(aScene);
		mSceneLoaded.store(true);
	}
}

void VulkanRenderBackend::sceneUnload(const SceneBackendData aScene) {
	if (!mImplementations.empty()) {
		mSceneLoaded.store(false);
		
		while (mCmdRecording.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		mInstance.mDevice->waitIdle();
		mImplementations[mCurrentImplementationIndex]->sceneUnload(aScene);
	}
}

void VulkanRenderBackend::beginFrame() {
	mCmdRecording.store(true);
	mInstance.beginFrame();
}

void VulkanRenderBackend::drawView(ViewDef_s* aViewDef)
{
	aViewDef->headless = !mMainWindow;
	if (aViewDef->headless) aViewDef->target_size = var::varToVec(var::render_size);
	else {
		
		const VkExtent2D extent = mInstance.mSwapchainData.mSwapchain->getExtent();
		aViewDef->target_size = glm::ivec2(extent.width, extent.height);
	}
	if (mSceneLoaded.load() && !mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->drawView(aViewDef);
}

void VulkanRenderBackend::drawUI(UiConf_s* aUiConf)
{
	if (!mInstance.mSwapchainData.mSwapchain) return;
	const VulkanInstance::SwapchainData& sd = mInstance.mSwapchainData;
	CommandBuffer* cb = sd.mFrames[sd.mCurrentImageIndex].mCommandBuffer;
	rvk::Image* ci = mInstance.mSwapchainData.mSwapchain->getCurrentImage();
	mInstance.gui()->prepare(ci->getExtent().width, ci->getExtent().height);

	if (!var::hide_default_gui.value()) mMainGui.draw(aUiConf);
	if (aUiConf->system->getConfig().show_gui && !mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->drawUI(aUiConf);

	ci->CMD_TransitionImage(cb, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	const glm::vec3 cc = glm::vec3(var::bg.value()[0], var::bg.value()[1], var::bg.value()[2]) / 255.0f;
	cb->cmdBeginRendering({ { ci,{{cc.x, cc.y, cc.z, 1.0f}} , mSceneLoaded.load() ? RVK_L2 : RVK_LC, RVK_S2 } });
	mInstance.gui()->draw(cb, sd.mCurrentImageIndex);
	cb->cmdEndRendering();
	ci->CMD_TransitionImage(cb, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void VulkanRenderBackend::captureSwapchain(ScreenshotInfo_s* aScreenshotInfo)
{
	const VulkanInstance::SwapchainData& sd = mInstance.mSwapchainData;
	const VulkanInstance::SwapchainFrameData id = sd.mFrames[sd.mCurrentImageIndex];
	const VkExtent3D extent = id.mColor->getExtent();
	const bool alpha = false;
	aScreenshotInfo->width = extent.width;
	aScreenshotInfo->height = extent.height;
	aScreenshotInfo->channels = alpha ? 4 : 3;
	SingleTimeCommand stc(mInstance.mCommandPools[0], mInstance.mDevice->getQueue(0,1));
	aScreenshotInfo->data = rvk::swapchain::readPixelsScreen(&stc, mInstance.mDevice, id.mColor, alpha);
}

void VulkanRenderBackend::endFrame() {
	
	mInstance.endFrame();
	mCmdRecording.store(false);
}

void VulkanRenderBackend::prepare() const
{
	const VulkanInstance::SwapchainData& sd = mInstance.mSwapchainData;
	const VulkanInstance::SwapchainFrameData id = sd.mFrames[sd.mCurrentImageIndex];

	RenderInfo_s ri {};
	ri.headless = !mMainWindow;
	if (ri.headless) ri.targetSize = var::varToVec(var::render_size);
	else {
		const VkExtent3D extent = id.mColor->getExtent();
		ri.targetSize = glm::ivec2{ extent.width, extent.height };
	}
	if (!mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->prepare(ri);
}

void VulkanRenderBackend::destroy() const
{
	if (!mImplementations.empty()) mImplementations[mCurrentImplementationIndex]->destroy();
}
