#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/input.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/scene/camera.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/render_cmd_system.hpp>
#include <tamashii/core/render/render_system.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/topology/topology.hpp>
#include <tamashii/core/platform/system.hpp>
#include <tamashii/core/platform/filewatcher.hpp>

#include <glm/gtc/color_space.hpp>
#include <chrono>
#include <filesystem>

T_USE_NAMESPACE
std::atomic_bool Common::mFileDialogRunning = false;
void Common::init(const int aArgc, char* aArgv[], const char* aCmdline)
{
	
	initLogger();

	
	sys::initSystem(mSystemInfo);

	
	try {
		ccli::parseArgs(aArgc, aArgv);
	}
	catch (ccli::CCLIError& e) {
		spdlog::error("Could not parse command line arguments: {}", e.message());
	}

	spdlog::info("Starting " + var::program_name.value());

	
	mConfigCache = ccli::loadConfig(var::cfg_filename.value());
	
	setStartupVariables();

	std::filesystem::current_path(var::work_dir.value());
	spdlog::info("Working Dir: '{}'", std::filesystem::current_path().string());
	std::filesystem::create_directories(var::cache_dir.value() + "/shader");
	spdlog::info("Cache Dir: '{}'", std::filesystem::path(std::filesystem::current_path().string() + "/" + var::cache_dir.value()).make_preferred().string());
	spdlog::info("Config: '{}'", std::filesystem::path(std::filesystem::current_path().string() + "/" + var::cfg_filename.value()).make_preferred().string());

	Window* mw = nullptr;
	if (!var::headless.value()) {
		mw = mWindow.window();
		mw->init(var::window_title.value().c_str(), var::varToVec(var::window_size), var::varToVec(var::window_pos));
	}

	
	
	if (mWindow.isWindow() && var::window_thread.value() && !mFrameThread.joinable()) {
		std::condition_variable cvSetupDone;
		std::condition_variable cvStartRendering;
		spdlog::info("...using separate window thread");
		spdlog::info("Initializing renderer");
		mFrameThread = std::thread([&] {
			
			mRenderSystem.init(mw);
			ccli::executeCallbacks();
			EventSystem::getInstance().eventLoop();
			cvSetupDone.notify_all();

			std::mutex mutex;
			std::unique_lock lck(mutex);
			cvStartRendering.wait(lck);
			while (frameIntern()) {
				const std::lock_guard<std::mutex> lg(mRenderFrameMutex);
			}
			mRenderSystem.shutdown();
		});

		std::mutex mutex;
		std::unique_lock<std::mutex> lck(mutex);
		cvSetupDone.wait(lck);
		cvStartRendering.notify_all();
	} else {
		spdlog::info("Initializing renderer");
		
		mRenderSystem.init(mw);
		ccli::executeCallbacks();
		EventSystem::getInstance().eventLoop();
	}
	
	FileWatcher& watcher = FileWatcher::getInstance();
	mFileWatcherThread = watcher.spawn();
	if (mw) mw->showWindow(var::window_maximized.value() ? Window::Show::max : Window::Show::yes);
}

void Common::initLogger() {
	if (!didInitLogger) {
		spdlog::set_pattern("%H:%M:%S.%e %^%l%$ %v");
		spdlog::set_level(spdlog::level::from_str(var::logLevel.value()));
		didInitLogger = true;
	}
}

void Common::shutdown()
{
	shutdownIntern();
}

void Common::queueShutdown()
{
	mShutdown = true;
}

bool Common::frame()
{
	
	mWindow.frame();
	
	if(!mFrameThread.joinable()) frameIntern();
	return !mShutdown;
}

ScreenshotInfo_s Common::screenshot()
{
	const std::lock_guard<std::mutex> lg(mRenderFrameMutex);
	ScreenshotInfo_s si = {};
	mRenderSystem.captureSwapchain(&si);
	return si;
}

Common::Common() : mSystemInfo{}, mShutdown{ false }, mIntern{},
	mIntersectionSettings{ CullMode::None, HitMask::All }, mDrawInfo{}, mScreenshotSettings{ 0 }
{}

void Common::shutdownIntern()
{
	spdlog::info("Shutting down " + var::program_name.value());
	mIntern.mShutdown = true;

	
	if (!var::headless) {
		if (!mWindow.isMaximized()) {
			var::window_pos.value(var::vecToVar(mWindow.getPosition()));
			var::window_size.value(var::vecToVar(mWindow.getSize()));
		}
		var::window_maximized.value(mWindow.isMaximized());
	}
	ccli::writeConfig(var::cfg_filename.value(), mConfigCache);
	
	FileWatcher::getInstance().terminate();
	if (mFileWatcherThread.joinable()) mFileWatcherThread.join();
	if (mFrameThread.joinable()) mFrameThread.join();
	else if(mRenderSystem.isInit()) mRenderSystem.shutdown();
	if (mRenderThread.joinable()) mRenderThread.join();
	if (mWindow.isWindow()) mWindow.shutdown();
	mShutdown = false;
}

bool Common::frameIntern()
{
	
	EventSystem::getInstance().eventLoop();
	if (mIntern.mShutdown) {
		mIntern = {};
		return false;
	}
	if (mIntern.mOpenMainWindow) {
		if(!mWindow.window()->isWindow()) mWindow.window()->init(var::window_title.value().c_str(), var::varToVec(var::window_size), var::varToVec(var::window_pos));
		mRenderSystem.registerMainWindow(mWindow.window());
		mWindow.showWindow(true);
		mIntern.mOpenMainWindow = false;
	}
	if (mIntern.mCloseMainWindow) {
		if (mWindow.window()->isWindow()) {
			mWindow.showWindow(false);
			mRenderSystem.unregisterMainWindow(mWindow.window());
			mWindow.window()->destroy();
		}
		mIntern.mCloseMainWindow = false;
	}
	
	if (mWindow.isMinimized()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		return true;
	}

	processInputs();
	while (renderCmdSystem.frames() >= 1) {}

	
	renderCmdSystem.addBeginFrameCmd();
	auto& main_scene = mRenderSystem.getMainScene();
	if (main_scene->getCurrentCamera().mode == RefCamera::Mode::EDITOR && mDrawInfo.mDrawMode && mDrawInfo.mHitInfo.mHit && mDrawInfo.mTarget == DrawInfo::Target::CUSTOM) renderCmdSystem.addDrawOnMeshCmd(&mDrawInfo, &mDrawInfo.mHitInfo);
	if (main_scene->animation()) main_scene->update(mRenderSystem.getConfig().frametime);
	main_scene->draw(); 

	const auto uc = new UiConf_s;
	
	
	uc->scene = main_scene.get();

	
	uc->system = &mRenderSystem;
	
	uc->draw_info = &mDrawInfo;
	renderCmdSystem.addDrawUICmd(uc);
	renderCmdSystem.addEndFrameCmd();

	bool reset_gui_settings = false;
	const bool show_gui = mRenderSystem.getConfig().show_gui;
	const bool mark_lights = mRenderSystem.getConfig().mark_lights;
	if (!mScreenshotPath.empty())
	{
		reset_gui_settings = true;
		if ((mScreenshotSettings & SCREENSHOT_NO_UI) > 0) mRenderSystem.getConfig().show_gui = false;
		if ((mScreenshotSettings & SCREENSHOT_NO_LIGHTS_OVERLAY) > 0) mRenderSystem.getConfig().mark_lights = false;
		if (!mScreenshotPath.has_extension()) mScreenshotPath = mScreenshotPath.string() + ".png";
		renderCmdSystem.addScreenshotCmd(mScreenshotPath.string());
		spdlog::info("Screenshot saved: {}", mScreenshotPath.string());
		mScreenshotPath.clear();
	}

	
	if (!var::render_thread.value()) mRenderSystem.processCommands();
	else if (!mRenderThread.joinable()) {
		spdlog::warn("Using separate frame/render thread -> very experimental");
		mRenderThread = std::thread([&] { while (!mShutdown) mRenderSystem.processCommands(); });
	}

	if(reset_gui_settings) {
		mRenderSystem.getConfig().show_gui = show_gui;
		mRenderSystem.getConfig().mark_lights = mark_lights;
	}
	return true;
}

void Common::setStartupVariables(const std::string& aMatch)
{
    if(glm::any(glm::equal(var::varToVec(var::window_size), glm::ivec2(0)))) var::window_size.value({1280, 720});
	mDrawInfo.mCursorColor = glm::vec3(1);
	mDrawInfo.mRadius = 0.1f;
	mDrawInfo.mColor0 = glm::vec4(1, 1, 1, 1);
	mDrawInfo.mDrawRgb = true;
	mDrawInfo.mDrawAlpha = true;
}

void Common::processInputs()
{
	const InputSystem& is = InputSystem::getInstance();
	
	if (is.isDown(Input::KEY_LCTRL))
	{
		mWindow.ungrabMouse();
		if (is.wasPressed(Input::KEY_O)) openFileDialogOpenScene();
		else if (is.wasPressed(Input::KEY_A)) openFileDialogAddScene();
		else if (is.wasPressed(Input::KEY_M)) openFileDialogOpenModel();
		else if (is.wasPressed(Input::KEY_L)) openFileDialogOpenLight();
	}

	auto& mainScene = mRenderSystem.getMainScene();
	if (!mainScene) return;
	
	if (mainScene->getCurrentCamera().mode == RefCamera::Mode::FPS) {
		if (is.wasReleased(Input::MOUSE_LEFT)) mWindow.grabMouse();
		if (is.wasPressed(Input::KEY_ESCAPE)) mWindow.ungrabMouse();
	}
	
	if (mainScene->getCurrentCamera().mode == RefCamera::Mode::EDITOR) {
		
		if (is.wasPressed(Input::KEY_ESCAPE)) {
			mainScene->setSelection({});
			mDrawInfo.mDrawMode = mDrawInfo.mHoverOver = false;
		}

		if (mDrawInfo.mDrawMode) {
			if (is.isDown(Input::KEY_LCTRL) && is.getMouseWheelRelative().y != 0.0f) {
				mDrawInfo.mRadius += 0.01f * is.getMouseWheelRelative().y;
			}

			Intersection& hi = mDrawInfo.mHitInfo;
			hi = {};
			intersectScene({ mIntersectionSettings.mCullMode, HitMask::Geometry }, &hi);
			if (hi.mHit) {
				const triangle_s tri = hi.mRefMeshHit->mesh->getTriangle(hi.mPrimitiveIndex, &hi.mHit->model_matrix);
				mDrawInfo.mHoverOver = true;
				mDrawInfo.mPositionWs = hi.mHitPos;
				mDrawInfo.mNormalWsNorm = tri.mGeoN;
				mDrawInfo.mTangentWsNorm = topology::calcStarkTangent(mDrawInfo.mNormalWsNorm);

				if (mDrawInfo.mTarget != DrawInfo::Target::CUSTOM) paintOnMesh(&hi);
			}
			else mDrawInfo.mHoverOver = false;
		}
		
		else {
			if (is.wasReleased(Input::KEY_D)) {
				mDrawInfo.mDrawMode = true;
				mainScene->setSelection({});
			}
			if (is.wasReleased(Input::MOUSE_LEFT)) {
				Intersection hi = {};
				intersectScene(mIntersectionSettings, &hi);
				mainScene->setSelection({ hi.mHit, hi.mMeshIndex, hi.mPrimitiveIndex, 0 });
			}
			const auto s = mainScene->getSelection().reference;
			if (is.wasPressed(Input::KEY_X) && s) {
				if (s->type == Ref::Type::Light) {
					mainScene->removeLight(std::dynamic_pointer_cast<RefLight>(s));
				}
				else if (s->type == Ref::Type::Model) {
					mainScene->removeModel(std::dynamic_pointer_cast<RefModel>(s));
				}
				mainScene->setSelection({});
			}
		}
	}
	else mainScene->setSelection({});

	RenderConfig_s& rc = mRenderSystem.getConfig();
	if (is.wasPressed(Input::KEY_F1)) rc.show_gui = !rc.show_gui;
	if (is.wasPressed(Input::KEY_F2)) rc.mark_lights = !rc.mark_lights;
	if (is.wasPressed(Input::KEY_F5)) {
		const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		const std::string name = std::filesystem::path(std::filesystem::current_path().string() + "/" + SCREENSHOT_FILE_NAME + "_" + std::to_string(now.time_since_epoch().count())).make_preferred().string();
		mScreenshotPath = name + ".png";
	}
	if (is.wasPressed(Input::KEY_F6)) renderCmdSystem.addImplScreenshotCmd();

	
	auto& refCam = mainScene->getCurrentCamera();
	const glm::ivec2 surfaceSize = mWindow.isWindow() ? mWindow.getSize() : var::varToVec(var::render_size);
	bool camUpdated = false;
	
	auto& cam = *refCam.camera;
	if (cam.getType() == Camera::Type::PERSPECTIVE) camUpdated |= cam.updateAspectRatio(surfaceSize);
	else if (cam.getType() == Camera::Type::ORTHOGRAPHIC) camUpdated |= cam.updateMag(surfaceSize);
	
	auto& refCamPrivate = static_cast<RefCameraPrivate&>(refCam);
	const glm::vec2 mousePosRelative = is.getMousePosRelative();
	const glm::vec2 mouseWheelRelative = is.getMouseWheelRelative();
	if(refCam.mode == RefCamera::Mode::FPS && mWindow.isMouseGrabbed())
	{
		camUpdated |= refCamPrivate.updatePosition(is.isDown(Input::KEY_W), is.isDown(Input::KEY_S),
			is.isDown(Input::KEY_A), is.isDown(Input::KEY_D), is.isDown(Input::KEY_E), is.isDown(Input::KEY_Q));
		camUpdated |= refCamPrivate.updateAngles(mousePosRelative.x, mousePosRelative.y);
		camUpdated |= refCamPrivate.updateSpeed(mouseWheelRelative.y);
	} else if (refCam.mode == RefCamera::Mode::EDITOR)
	{
		if (is.isDown(Input::KEY_LSHIFT) && is.isDown(Input::MOUSE_WHEEL)) camUpdated |= refCamPrivate.updateCenter(mousePosRelative);
		else if (is.isDown(Input::MOUSE_WHEEL)) camUpdated |= refCamPrivate.drag(glm::vec2(mousePosRelative));
		else if (!is.isDown(Input::KEY_LCTRL)) camUpdated |= refCamPrivate.updateZoom(mouseWheelRelative.y);
	}
	if (camUpdated) {
		refCamPrivate.updateMatrix(true);
		mainScene->requestCameraUpdate();
	}
}

void Common::paintOnMesh(const Intersection *aHitInfo) const
{
	if (mDrawInfo.mTarget == DrawInfo::Target::VERTEX_COLOR) {
		auto color = glm::vec4(0.0f);
		if (InputSystem::getInstance().isDown(Input::MOUSE_LEFT)) color = mDrawInfo.mColor0;
		else if (InputSystem::getInstance().isDown(Input::MOUSE_RIGHT)) color = mDrawInfo.mColor1;
		else return;

		
		color = glm::vec4(glm::convertSRGBToLinear(glm::vec3(color)), color.w);

		const glm::vec3 hitN = glm::normalize(aHitInfo->mRefMeshHit->mesh->getTriangle(aHitInfo->mPrimitiveIndex).mGeoN);
		
		const auto& refModel = dynamic_cast<RefModel&>(*(aHitInfo->mHit));
		for (const auto& mesh : *refModel.model) {
			std::vector<vertex_s>* vertices = mesh->getVerticesVector();
			for (vertex_s& v : *vertices) {
				auto newColor = glm::vec4(0);
				const float ndotn = glm::dot(hitN, glm::vec3(v.normal));
				if (mDrawInfo.mDrawAll) {}
				else {
					
					if (ndotn < 0) continue;
					const float dist = glm::length(glm::vec3(refModel.model_matrix * v.position) - mDrawInfo.mPositionWs);
					if (dist <= mDrawInfo.mRadius) {
						if (mDrawInfo.mSoftBrush) {
							const float ratio = dist / mDrawInfo.mRadius;
							glm::mix(v.color_0, color, ratio);
							if (InputSystem::getInstance().isDown(Input::MOUSE_LEFT)) newColor = glm::mix(color, v.color_0, ratio * ndotn);
							else if (InputSystem::getInstance().isDown(Input::MOUSE_RIGHT)) newColor = glm::mix(color, v.color_0, ratio * ndotn);
						}
					}
					else continue;
				}

				if (mDrawInfo.mDrawRgb) {
					v.color_0.x = mDrawInfo.mSoftBrush ? newColor.x : color.x;
					v.color_0.y = mDrawInfo.mSoftBrush ? newColor.y : color.y;
					v.color_0.z = mDrawInfo.mSoftBrush ? newColor.z : color.z;
				}
				if (mDrawInfo.mDrawAlpha) v.color_0.w = mDrawInfo.mSoftBrush ? newColor.w : color.w;
			}
		}
	}
	else return;
	auto& mainScene = mRenderSystem.getMainScene();
	mainScene->requestModelGeometryUpdate();
}

void Common::newScene()
{
	
	FileWatcher::getInstance().removeFile(mRenderSystem.getMainScene()->getSceneFileName());
	
	mRenderSystem.getMainScene()->readyToRender(false);
	mRenderSystem.sceneUnload(mRenderSystem.getMainScene()->getSceneData());
	mRenderSystem.freeMainRenderScene();

	
	const auto scene = mRenderSystem.allocRenderScene();
	mRenderSystem.setMainRenderScene(scene);
	mRenderSystem.sceneLoad(scene->getSceneData());
	scene->readyToRender(true);
}
void Common::openScene(const std::string& aFile)
{
	
	FileWatcher::getInstance().removeFile(mRenderSystem.getMainScene()->getSceneFileName());
	
	mRenderSystem.getMainScene()->readyToRender(false);
	mRenderSystem.sceneUnload(mRenderSystem.getMainScene()->getSceneData());
	mRenderSystem.freeMainRenderScene();

	
	auto scene = mRenderSystem.allocRenderScene();
	{
		if (scene->initFromFile(aFile)) {
			mRenderSystem.sceneLoad(scene->getSceneData());

			
			FileWatcher::getInstance().watchFile(aFile, [scene, aFile, this]() {
				printf("File watcher is doing stuff\n");
				
				const std::string cam = scene->getCurrentCamera().camera->getName().data();
				
				scene->readyToRender(false);
				mRenderSystem.sceneUnload(scene->getSceneData());
				
				scene->destroy();
				
				scene->initFromFile(aFile);
				
				for (const auto& c : scene->getAvailableCameras()) {
					if (cam == c->camera->getName()) { scene->setCurrentCamera(c); break; }
				}
				
				mRenderSystem.sceneLoad(scene->getSceneData());
				scene->readyToRender(true);
				});
		}
		scene->readyToRender(true);
	}
	mRenderSystem.setMainRenderScene(scene);
}

void Common::addScene(const std::string& aFile) const
{
	auto& rs = mRenderSystem.getMainScene();
	rs->readyToRender(false);
	mRenderSystem.sceneUnload(rs->getSceneData());
	if(!rs->addSceneFromFile(aFile))
	{
		spdlog::error("Could not add scene");
	}
	rs->readyToRender(true);
	mRenderSystem.sceneLoad(rs->getSceneData());
}

void Common::addModel(const std::string& aFile) const
{
	auto m = io::Import::load_model(aFile);
	glm::quat quat = glm::quat(glm::radians(glm::vec3(-90, 0, 0)));
	mRenderSystem.getMainScene()->addModelRef(std::move(m), glm::vec3(0), { quat[1], quat[2], quat[3] , quat[0] });
	
	mRenderSystem.reloadCurrentScene();
}
void Common::addLight(const std::string& aFile) const
{
	auto l = io::Import::load_light(aFile);
	glm::quat quat = glm::quat(glm::radians(glm::vec3(-90, 0, 0)));
	mRenderSystem.getMainScene()->addLightRef(std::move(l), glm::vec3(0), { quat[1], quat[2], quat[3] , quat[0] });

}

void Common::exportScene(const std::string& aOutputFile, const uint32_t aSettings) const
{
	const io::Export::SceneExportSettings exportSettings(aSettings);
	const io::SceneData si = mRenderSystem.getMainScene()->getSceneInfo();
	io::Export::save_scene(aOutputFile, exportSettings, si);
}

void Common::reloadBackendImplementation() const
{
	mRenderSystem.reloadBackendImplementation();
}

void Common::changeBackendImplementation(const int aIdx)
{
	mIntersectionSettings = IntersectionSettings();
	mRenderSystem.getMainScene()->setSelection({}); 
	mRenderSystem.changeBackendImplementation(aIdx);	
}

bool Common::changeBackendImplementation(const char* name)
{
	mIntersectionSettings = IntersectionSettings();
	mRenderSystem.getMainScene()->setSelection({}); 
	return mRenderSystem.changeBackendImplementation(name);	
}

void tamashii::Common::clearCache() const
{
	std::filesystem::remove_all(var::cache_dir.value());
	std::filesystem::create_directories(var::cache_dir.value() + "/shader");
	spdlog::info("Cache cleared: '{}'", var::cache_dir.value());
}

void Common::queueScreenshot(const std::filesystem::path& aOut, uint32_t aScreenshotSettings)
{
	mScreenshotPath = aOut;
	mScreenshotSettings = aScreenshotSettings;
}

void Common::intersectScene(const IntersectionSettings aSettings, Intersection* aHitInfo)
{
	auto& scene = mRenderSystem.getMainScene();
	if (!scene) return;
	const glm::vec2 mouse_coordinates = InputSystem::getInstance().getMousePosAbsolute();
	const glm::vec2 uv = (mouse_coordinates + glm::vec2(0.5f)) / glm::vec2(mWindow.getSize());
	const glm::vec2 screenCoordClipSpace = uv * glm::vec2(2.0) - glm::vec2(1.0);

	auto& cam = reinterpret_cast<RefCameraPrivate&>(scene->getCurrentCamera());
	glm::vec4 dirViewSpace = glm::inverse(cam.camera->getProjectionMatrix()) * glm::vec4(screenCoordClipSpace, 1, 1);
	dirViewSpace = glm::vec4(glm::normalize(glm::vec3(dirViewSpace)), 0);
	const auto dirWorldSpace = glm::vec3(glm::inverse(cam.view_matrix) * dirViewSpace);

	const glm::vec3 posWorldSpace = cam.getPosition();
	aHitInfo->mOriginPos = posWorldSpace;
	scene->intersect(posWorldSpace, dirWorldSpace, aSettings, aHitInfo);
}

RenderSystem* Common::getRenderSystem()
{ return &mRenderSystem; }

WindowThread* Common::getMainWindow()
{ return mWindow.isWindow() ? &mWindow : nullptr; }

void Common::openMainWindow()
{
	mIntern.mOpenMainWindow = true;
	/*if (!mFrameThread.joinable()) {
		frame();
		frame();
	}*/
}

void Common::closeMainWindow()
{
	mIntern.mCloseMainWindow = true;
}

IntersectionSettings& Common::intersectionSettings()
{
	return mIntersectionSettings;
}

void Common::openFileDialogOpenScene()
{
	if (mFileDialogRunning.load()) return;
	mFileDialogRunning.store(true);
	EventSystem::getInstance().reset();
	auto t = std::thread([]() {
		const std::filesystem::path path = sys::openFileDialog("Open Scene", io::Import::instance().load_scene_file_dialog_info());
		if (!path.string().empty()) EventSystem::queueEvent(EventType::ACTION, Input::A_OPEN_SCENE, 0, 0, 0, path.string());
		mFileDialogRunning.store(false);
	});
	t.detach();
}

void Common::openFileDialogAddScene()
{
	if (mFileDialogRunning.load()) return;
	mFileDialogRunning.store(true);
	EventSystem::getInstance().reset();
	auto t = std::thread([]() {
		const std::filesystem::path path = sys::openFileDialog("Add Scene", io::Import::instance().load_scene_file_dialog_info());
		if (!path.string().empty()) EventSystem::queueEvent(EventType::ACTION, Input::A_ADD_SCENE, 0, 0, 0, path.string());
		mFileDialogRunning.store(false);
	});
	t.detach();
}

void Common::openFileDialogOpenModel()
{
	if (mFileDialogRunning.load()) return;
	mFileDialogRunning.store(true);
	EventSystem::getInstance().reset();
	auto t = std::thread([]() {
		const std::filesystem::path path = sys::openFileDialog("Add Model", { { "Model", {"*.ply", "*.obj"} },
																   { "PLY (.ply)", {"*.ply"} },
																   { "OBJ (.obj)", {"*.obj"} } });
		if (!path.string().empty()) EventSystem::queueEvent(EventType::ACTION, Input::A_ADD_MODEL, 0, 0, 0, path.string());
		mFileDialogRunning.store(false);
	});
	t.detach();
}

void Common::openFileDialogOpenLight()
{
	if (mFileDialogRunning.load()) return;
	mFileDialogRunning.store(true);
	EventSystem::getInstance().reset();
	auto t = std::thread([]() {
		const std::filesystem::path path = sys::openFileDialog("Add Light", { { "Light", {"*.ies", "*.ldt"} },
																   { "IES (.ies)", {"*.ies"} },
																   { "LDT (.ldt)", {"*.ldt"} } });
		if (!path.string().empty()) EventSystem::queueEvent(EventType::ACTION, Input::A_ADD_LIGHT, 0, 0, 0, path.string());
		mFileDialogRunning.store(false);
	});
	t.detach();
}

void Common::openFileDialogExportScene(const uint32_t aSettings)
{
	if (mFileDialogRunning.load()) return;
	mFileDialogRunning.store(true);
	EventSystem::getInstance().reset();
	const io::Export::SceneExportSettings exportSettings(aSettings);
	auto t = std::thread([exportSettings]() {
		std::string location;
		switch (exportSettings.mFormat) {
		case io::Export::SceneExportSettings::Format::glTF:
		{
			if (exportSettings.mWriteBinary) location = sys::saveFileDialog("Save Scene", "glb").string();
			else location = sys::saveFileDialog("Save Scene", "gltf").string();
			break;
		}
		}
		if (!location.empty()) EventSystem::queueEvent(EventType::ACTION, Input::A_EXPORT_SCENE, static_cast<int>(exportSettings.encode()), 0, 0, location);
		mFileDialogRunning.store(false);
	});
	t.detach();
}
