#include <tamashii/core/render/render_system.hpp>
#include <tamashii/core/render/render_backend.hpp>
#include <tamashii/core/render/render_backend_implementation.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/io/io.hpp>

#include <array>
#include <chrono>

T_USE_NAMESPACE

std::deque<std::shared_ptr<RenderBackend>> RenderSystem::mRegisteredBackends;

RenderSystem::RenderSystem() : mInit(false), mRenderConfig(), mMainScene(nullptr), mCurrendRenderBackend(nullptr)
{
	mRenderBackends.reserve(3);
}

RenderSystem::~RenderSystem()
= default;

RenderConfig_s& RenderSystem::getConfig()
{ return mRenderConfig; }

bool RenderSystem::isInit() const
{ return mInit; }

bool RenderSystem::drawOnMesh(const DrawInfo* aDrawInfo) const
{ return mCurrendRenderBackend->drawOnMesh(aDrawInfo); }

const std::shared_ptr<RenderScene>& RenderSystem::getMainScene() const
{ return mMainScene; }

void RenderSystem::setMainRenderScene(std::shared_ptr<RenderScene> aRenderScene)
{ mMainScene = std::move(aRenderScene); }

void RenderSystem::sceneLoad(const SceneBackendData& aScene) const
{ if(mCurrendRenderBackend) mCurrendRenderBackend->sceneLoad(aScene); }

void RenderSystem::sceneUnload(const SceneBackendData& aScene) const
{ if(mCurrendRenderBackend) mCurrendRenderBackend->sceneUnload(aScene); }

void RenderSystem::addImplementation(RenderBackendImplementation* aImplementation) const
{ if(mCurrendRenderBackend) mCurrendRenderBackend->addImplementation(aImplementation); }

void RenderSystem::reloadBackendImplementation() const
{
	const SceneBackendData s = mMainScene->getSceneData();
	mCurrendRenderBackend->reloadImplementation(s);
}

std::vector<RenderBackendImplementation*>& RenderSystem::getAvailableBackendImplementations() const
{ return mCurrendRenderBackend->getAvailableBackendImplementations(); }

RenderBackendImplementation* RenderSystem::getCurrentBackendImplementations() const
{ return mCurrendRenderBackend->getCurrentBackendImplementations(); }

RenderBackendImplementation* RenderSystem::findBackendImplementation(const char* name) const
{
	for (const auto impl : mCurrendRenderBackend->getAvailableBackendImplementations())
	{
		if (strcmp(impl->getName(), name) == 0) {
			return impl;
		}
	}
	return nullptr;
}






void RenderSystem::init(tamashii::Window* aWindow)
{
	mRenderConfig.show_gui = true;
	mRenderConfig.mark_lights = true;
	mRenderConfig.frame_index = 0;

	mMainScene = allocRenderScene();
	mMainScene->readyToRender(true);

	for(auto& be : mRegisteredBackends)
	{
		mRenderBackends.push_back(be);
	}

	if(mRenderBackends.empty())
	{
		spdlog::error("No render backend set!");
		Common::getInstance().shutdown();
		return;
	}
	mCurrendRenderBackend = mRenderBackends.front();
	for(const auto& rb : mRenderBackends) {
		if (var::render_backend.value() == rb->getName()) {
			mCurrendRenderBackend = rb;
			break;
		}
	}
	mCurrendRenderBackend->init(aWindow);
	mInit = true;
}

void RenderSystem::registerMainWindow(tamashii::Window* aWindow) const
{
	mCurrendRenderBackend->registerMainWindow(aWindow);
}

void RenderSystem::unregisterMainWindow(tamashii::Window* aWindow) const
{
	mCurrendRenderBackend->unregisterMainWindow(aWindow);
}

void RenderSystem::shutdown()
{
	if (!mInit) return;
	for (auto& rs : mScenes) {
		sceneUnload(rs->getSceneData());
		rs.reset();
	}
	mScenes.clear();
	mMainScene.reset();
	if (mCurrendRenderBackend) mCurrendRenderBackend->shutdown();
	mRenderBackends.clear();
	mInit = false;
}

void RenderSystem::renderSurfaceResize(const uint32_t aWidth, const uint32_t aHeight) const
{
	mCurrendRenderBackend->recreateRenderSurface(aWidth, aHeight);
}

void RenderSystem::processCommands()
{
	RCmd_s cmd{ RenderCommand::EMPTY, {} };
	while(renderCmdSystem.nextCmd()) {
		cmd = renderCmdSystem.popNextCmd();
		switch (cmd.mType) {
			case RenderCommand::BEGIN_FRAME:
				updateStatistics();
				mCurrendRenderBackend->beginFrame();
				break;
			case RenderCommand::DRAW_VIEW:
				std::get<ViewDef_s*>(cmd.mValue)->frame_index = mRenderConfig.frame_index;
				mCurrendRenderBackend->drawView(std::get<ViewDef_s*>(cmd.mValue));
				break;
			case RenderCommand::DRAW_UI:
				mCurrendRenderBackend->drawUI(std::get<UiConf_s*>(cmd.mValue));
				break;
			case RenderCommand::ENTITY_ADDED:
				mCurrendRenderBackend->entitiyAdded(*std::get<std::shared_ptr<Ref>>(cmd.mValue));
				break;
			case RenderCommand::ENTITY_REMOVED:
				mCurrendRenderBackend->entitiyRemoved(*std::get<std::shared_ptr<Ref>>(cmd.mValue));
				break;
			case RenderCommand::SCREENSHOT:
			{
				const auto& si = std::get<ScreenshotInfo_s*>(cmd.mValue);
				mCurrendRenderBackend->captureSwapchain(si);
				io::Export::save_image_png_8_bit(si->name, si->width, si->height, si->channels, si->data.data());
				break;
			}
			case RenderCommand::IMPL_SCREENSHOT:
			{
				std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
				mCurrendRenderBackend->screenshot(std::to_string(now.time_since_epoch().count()));
				break;
			}
			case RenderCommand::IMPL_DRAW_ON_MESH:
			{
				bool result = mCurrendRenderBackend->drawOnMesh(std::get<DrawInfo*>(cmd.mValue));
				break;
			}
			case RenderCommand::END_FRAME:
			{
				std::optional<std::lock_guard<std::mutex>> optionalLock;
				if(Common::getInstance().getMainWindow()) optionalLock.emplace(Common::getInstance().getMainWindow()->window()->resizeMutex());
				mCurrendRenderBackend->endFrame();
				optionalLock.reset();
				break;
			}
			case RenderCommand::ASSET_ADDED:
			case RenderCommand::ASSET_REMOVED:
			case RenderCommand::EMPTY: break;
		}
		renderCmdSystem.deleteCmd(cmd);
	}
}

std::shared_ptr<RenderScene> RenderSystem::allocRenderScene()
{
	mScenes.emplace_back(std::make_shared<RenderScene>());
	return mScenes.back();
}

void RenderSystem::freeMainRenderScene()
{
	removeRenderScene(mMainScene);
	mMainScene.reset();
}

void RenderSystem::removeRenderScene(const std::shared_ptr<RenderScene>& aRenderScene)
{
	if (!aRenderScene) return;
	mScenes.remove(aRenderScene);
}

void RenderSystem::reloadCurrentScene() const
{
	const SceneBackendData s = mMainScene->getSceneData();
	mMainScene->readyToRender(false);
	sceneUnload(s);
	sceneLoad(s);
	mMainScene->readyToRender(true);
}

void RenderSystem::captureSwapchain(ScreenshotInfo_s* aScreenshotInfo) const
{
	mCurrendRenderBackend->captureSwapchain(aScreenshotInfo);
}

void RenderSystem::changeBackendImplementation(const int aIndex) const
{
	const SceneBackendData s = mMainScene->getSceneData();
	mCurrendRenderBackend->changeImplementation(aIndex, s);
}

bool RenderSystem::changeBackendImplementation(const char* name) const
{
	const auto backends = getAvailableBackendImplementations();
	int idx = 0;
	for (const auto backend : backends)
	{
		if (std::strcmp(backend->getName(), name) == 0) {
			changeBackendImplementation(idx);
			return true;
		}
		++idx;
	}
	return false;
}

void RenderSystem::updateStatistics()
{
	mRenderConfig.frame_index++;

	
	{
		static std::array<float, FPS_FRAMES> previousTimes;
		static uint32_t index;
		static std::chrono::time_point<std::chrono::high_resolution_clock> previous;

		std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();

		
		if (var::max_fps.value()) {
			const float target = (1.0f / static_cast<float>(var::max_fps.value())) * 1000.0f;
			while (std::chrono::duration<float, std::milli>(t - previous).count() < target) {
				t = std::chrono::high_resolution_clock::now();
			}
		}

		mRenderConfig.frametime = std::chrono::duration<float, std::milli>(t - previous).count();
		previous = t;

		mRenderConfig.frametime_smooth = 0;

		previousTimes[index % FPS_FRAMES] = mRenderConfig.frametime;
		index++;
		if (index > FPS_FRAMES)
		{
			
			mRenderConfig.frametime_smooth = 0;
			for (uint32_t i = 0; i < FPS_FRAMES; i++)
			{
				mRenderConfig.frametime_smooth += previousTimes[i];
			}
			mRenderConfig.frametime_smooth = mRenderConfig.frametime_smooth / FPS_FRAMES;
			mRenderConfig.framerate_smooth = 1000.0f / mRenderConfig.frametime_smooth;
		}
	}
}
