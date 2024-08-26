#include <tamashii/tamashii.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/core_module.hpp>
#include <tamashii/bindings/exports.hpp>
#include <tamashii/bindings/ccli_variable.hpp>
#include <tamashii/core/common/input.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/scene/ref_entities.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/render_cmd_system.hpp>
#include <tamashii/core/render/render_backend_implementation.hpp>

#include <fmt/color.h>

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS

using namespace std::literals;

namespace
{
    bool frame()
    {
        CoreModule::initGuard(); return Common::getInstance().frame();
    }

    void runWithWindow()
    {
        CoreModule::initGuard();
        Common::getInstance().openMainWindow();
    	while (Common::getInstance().frame()) {}
        Common::getInstance().closeMainWindow();
    }

    Scene newScene()
    {
        CoreModule::initGuard();
        Common::getInstance().newScene();
        return { Common::getInstance().getRenderSystem()->getMainScene() };
    }

    Scene openScene(const std::string& path)
    {
        CoreModule::initGuard();
        Common::getInstance().openScene(path);
        return { Common::getInstance().getRenderSystem()->getMainScene() };
    }

    Scene addScene(const std::string& path)
    {
        CoreModule::initGuard();
        Common::getInstance().addScene(path);
        return { Common::getInstance().getRenderSystem()->getMainScene() };
    }

    python::Image<uint8_t> captureFrame()
    {
        CoreModule::initGuard();
        auto screenshot = Common::getInstance().screenshot();

        if (screenshot.channels != 3) {
            CoreModule::exitWithError("Only screenshots with 3 channels are currently supported");
        }

        
        const auto vectorHandle = new std::vector<uint8_t>{ std::move(screenshot.data) };
        nb::capsule owner(vectorHandle, "Screenshot capsule", [](void* ptr) noexcept {
            delete static_cast<std::vector<uint8_t>*>(ptr);
        });

        size_t shape[3] = { screenshot.height, screenshot.width, 3 };
        return { vectorHandle->data(), 3, shape, owner };
    }

    python::Image<uint8_t> captureNextFrame()
    {
        frame();
        return captureFrame();
    }

    void saveScreenshot(const std::string& path, const bool showUI, const bool showLightOverlay)
    {
        CoreModule::initGuard();
        uint32_t flags = 0;
        flags |= (showUI ? 0 : SCREENSHOT_NO_UI);
        flags |= (showLightOverlay ? 0 : SCREENSHOT_NO_LIGHTS_OVERLAY);

        if (path.empty()) {
            Common::getInstance().queueScreenshot("screenshot.png", flags);
        }
        else {
            Common::getInstance().queueScreenshot(path, flags);
        }
    }

    void openWindow()
    {
        CoreModule::initGuard();
        Common::getInstance().openMainWindow();
        
        Common::getInstance().frame();
        Common::getInstance().frame();
    }
    void closeWindow()
    {
        CoreModule::initGuard();
        Common::getInstance().closeMainWindow();
        
        Common::getInstance().frame();
        Common::getInstance().frame();
    }
}

python::Exports::Exports(nanobind::module_& m)
{
    m.def("run", []() { CoreModule::initGuard(); while (Common::getInstance().frame()) {} });
    m.def("runWithWindow", &runWithWindow);

    m.def("frame", &frame);
    m.def("newScene", &newScene);
    m.def("openScene", &openScene, "path"_a);
    m.def("addScene", &addScene, "path"_a);

    m.def("captureCurrentFrame", &captureFrame);
    m.def("captureNextFrame", &captureNextFrame);
	

    m.def("openWindow", &openWindow);
    m.def("closeWindow", &closeWindow);

    m.def("scene", []()->Scene { CoreModule::initGuard(); return { Common::getInstance().getRenderSystem()->getMainScene() }; });
	m.def("impls", []()->Impls { CoreModule::initGuard(); return {}; });

	mImpls.emplace(m, "impls")
	.def_prop_rw_static("current",
	[](nb::handle) { return Common::getInstance().getRenderSystem()->getCurrentBackendImplementations()->getName(); },
	[](nb::handle, const char* impl) { (void)Common::getInstance().getRenderSystem()->changeBackendImplementation(impl); }
    );

	mVar.emplace(m, "var");
    ccli::forEachVar([&](ccli::VarBase& var, const size_t idx) -> ccli::IterationDecision {
        if (var.isBool())               python::attachVariable<bool>(*mVar, var);
        else if (var.isIntegral())      python::attachVariable<long long>(*mVar, var);
        else if (var.isFloatingPoint()) python::attachVariable<double>(*mVar, var);
        else if (var.isString())        python::attachVariable<std::string_view>(*mVar, var);
        else spdlog::error("Variable '{}' has unsupported type", var.longName());
        return {};
    });

	mScene.emplace(m, "Scene")
	.def_prop_ro("models", &Scene::getModels)
	.def_prop_ro("lights", &Scene::getLights)
	.def_prop_ro("cameras", &Scene::getCameras)
	.def("addLight", &Scene::addLight, "light"_a)
	.def_prop_ro("currentCamera", &Scene::getCurrentCamera);

	mModel.emplace(m, "Model")
	.def(nb::init())
	.def_prop_rw("name", &Model::getName, &Model::setName)
	.def_prop_ro("meshes", &Model::getMeshes);

	mMesh.emplace(m, "Mesh")
	.def(nb::init())
	.def_prop_rw("emissiveStrength", &Mesh::getEmissionStrength, &Mesh::setEmissionStrength)
	.def_prop_rw("emissiveFactor", &Mesh::getEmissionFactor, &Mesh::setEmissionFactor);

	mLight.emplace(m, "Light")
	.def(nb::init())
	.def_prop_rw("name", &Light::getName, &Light::setName)
	.def_prop_rw("position", &Light::getPosition, &Light::setPosition)
	.def_prop_rw("color", &Light::getColor, &Light::setColor)
	.def_prop_rw("intensity", &Light::getIntensity, &Light::setIntensity)
	.def_prop_rw("modelMatrix", &Light::getModelMatrix, &Light::setModelMatrix)
	.def("getType", &Light::getType)
	.def("isAttached", &Light::isAttached)
	.def("asPointLight", &Light::asPointLight)
	.def("asDirectionalLight", &Light::asDirectionalLight);

	mLightType.emplace(*mLight, "Type")
	.value("Directional", tamashii::Light::Type::DIRECTIONAL)
	.value("Point", tamashii::Light::Type::POINT)
	.value("Spot", tamashii::Light::Type::SPOT)
	.value("Surface", tamashii::Light::Type::SURFACE)
	.value("IES", tamashii::Light::Type::IES)
	.export_values();

	mPointLight.emplace(m, "PointLight")
	.def(nb::init())
	.def_prop_rw("radius", &PointLight::getRadius, &PointLight::setRadius)
	.def_prop_rw("range", &PointLight::getRange, &PointLight::setRange);

	mDirectionalLight.emplace(m, "DirectionalLight")
	.def(nb::init())
	.def_prop_rw("angle", &DirectionalLight::getAngle, &DirectionalLight::setAngle);

	mCamera.emplace(m, "Camera")
	.def(nb::init())
	.def_prop_rw("name", &Camera::getName, &Camera::setName)
	.def_prop_rw("position", &Camera::getPosition, &Camera::setPosition)
	.def_prop_rw("modelMatrix", &Camera::getModelMatrix, &Camera::setModelMatrix)
	.def("lookAt", &Camera::lookAt, "eyeVector"_a, "centerVector"_a)
	.def("isAttached", &Camera::isAttached)
	.def("makeCurrent", &Camera::makeCurrentCamera);
}

Exports& CoreModule::exportCore(nanobind::module_& m)
{
	CoreModule& cm = the();
    if (cm.mExports) exitWithError("Core module already defined");

	
	Common::getInstance().initLogger();
    cm.mScriptLogger = spdlog::default_logger()->clone("script");
    cm.mScriptLogger->set_level(spdlog::level::info);
    cm.mExports = std::make_unique<Exports>(m);
    auto& exports = *cm.mExports;

	
    var::headless.value(true);
    var::window_thread.value(false);
    var::window_thread.lock();
    var::logLevel.value("warn");

    m.def("log", [&](std::string_view msg) {
        cm.mScriptLogger->info("{} {}", fmt::format(fmt::fg(fmt::terminal_color::green), "script"), msg);
    });

    exitCallback([]() {
        the().mExports.reset();
        if (Common::getInstance().getRenderSystem()->isInit()) Common::getInstance().shutdown();
    });

    return exports;
}

void CoreModule::initGuard()
{
    if (!Common::getInstance().getRenderSystem()->isInit()) Common::getInstance().init(0, nullptr, nullptr);
}

void CoreModule::exitCallback(std::function<void()> cb)
{
    
    
    try {
        const auto atexit = nb::module_::import_("atexit");
        const auto registerFunction = nb::module_::import_("atexit").attr("register");
        registerFunction(nb::cpp_function(cb));
    }
    catch (nb::python_error& err) {
        exitWithError("Could not register atexit clean up code due to: {}\n\n", err.what());
    }
}

Scene::Scene(std::weak_ptr<tamashii::RenderScene> weakScene): renderScene{std::move(weakScene)}
{}

std::vector<std::unique_ptr<python::Model>> Scene::getModels() const
{
    const auto scene = tryGetRenderScene();

    const auto& models = scene->getModelList();
    std::vector<std::unique_ptr<python::Model>> pythonModels;
    pythonModels.reserve(models.size());
    for (auto& model : models) {
        pythonModels.emplace_back(std::make_unique<Model>(model));
    }
    return pythonModels;
}

std::vector<std::unique_ptr<python::Light>> python::Scene::getLights() const
{
	const auto scene= tryGetRenderScene();

    const auto& lights = scene->getLightList();
    std::vector<std::unique_ptr<Light>> pythonLights;
    pythonLights.reserve(lights.size());
    for (auto& light : lights) {
        pythonLights.emplace_back(Light::create(light));
    }

    return pythonLights;
}

std::vector<std::unique_ptr<python::Camera>> python::Scene::getCameras() const
{
    const auto scene = tryGetRenderScene();

    const auto& cameras = scene->getCameraList();
    std::vector<std::unique_ptr<Camera>> pythonCameras;
    pythonCameras.reserve(cameras.size());
    for (auto& camera : cameras) {
        pythonCameras.emplace_back(std::make_unique<Camera>(camera));
    }

    return pythonCameras;
}

void python::Scene::addLight(Light& pythonLight) const
{
	const auto scene = tryGetRenderScene();
    pythonLight.attachToScene( *scene );
}

python::Camera python::Scene::getCurrentCamera() const
{
    const auto scene = tryGetRenderScene();
    return python::Camera{ scene->referenceCurrentCamera() };
}

std::shared_ptr<tamashii::RenderScene> Scene::tryGetRenderScene() const
{
    auto scene= renderScene.lock();
    if (!scene) {
        CoreModule::exitWithError("Scene is not attached to the renderer.");
    }

    return scene;
}
