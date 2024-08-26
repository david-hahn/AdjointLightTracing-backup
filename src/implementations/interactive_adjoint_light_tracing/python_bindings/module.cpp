
#include <memory>
#include <nanobind/nanobind.h>
#include <tamashii/tamashii.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/core_module.hpp>
#include <tamashii/bindings/exports.hpp>
#include <tamashii/implementations/bindings/default_rasterizer.hpp>
#include <tamashii/implementations/bindings/default_path_tracer.hpp>

#include <glm/gtc/color_space.hpp>

#include "opt_param.hpp"

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS

namespace {
    constexpr const char* name = "ialt";
    class IALT
    {
    public:
        IALT()
        {
            CoreModule::initGuard();
            const auto impl = Common::getInstance().getRenderSystem()->findBackendImplementation(name);
            if (impl == nullptr) CoreModule::exitWithError("Expected \"{}\" to be a registered Impl", name);
            _impl = dynamic_cast<InteractiveAdjointLightTracing*>(impl);
        }

        [[maybe_unused]] IALT checkCurrent() const
        {
            if (Common::getInstance().getRenderSystem()->getCurrentBackendImplementations() != _impl)
            {
                CoreModule::exitWithError("Expected \"{}\" to be current Impl", name);
            }
            return *this;
        }

        [[nodiscard]] InteractiveAdjointLightTracing* handle() const { return _impl; }
    private:
        InteractiveAdjointLightTracing* _impl;
    };

    void setMeshTargetCW(const python::Mesh& self, const std::array<float, 3> c, float w)
    {
	    IALT().checkCurrent().handle()->getOptimizer().setTargetRadianceBufferForMesh(&self.refMesh(), glm::convertSRGBToLinear(glm::vec3{ c[0], c[1], c[2] }), w);
    };
    void setMeshTargetC(const python::Mesh& self, const std::array<float, 3> c)
    {
        IALT().checkCurrent().handle()->getOptimizer().setTargetRadianceBufferForMesh(&self.refMesh(), glm::convertSRGBToLinear(glm::vec3{ c[0], c[1], c[2] }), {});
    }

    void setSceneTargetCW(const python::Scene& self, const std::array<float, 3> c, float w)
    {
        IALT().checkCurrent().handle()->getOptimizer().setTargetRadianceBufferForScene(glm::convertSRGBToLinear(glm::vec3{ c[0], c[1], c[2] }), w);
    };
    void setSceneTargetC(const python::Scene& self, const std::array<float, 3> c)
    {
        IALT().checkCurrent().handle()->getOptimizer().setTargetRadianceBufferForScene(glm::convertSRGBToLinear(glm::vec3{ c[0], c[1], c[2] }), {});
    }
    
}

static python::Array<double> eigenVectorToPythonVector(Eigen::VectorXd dataVector, const char* name) {
    
    const auto vectorHandle = new Eigen::VectorXd{ std::move(dataVector) };
    nb::capsule owner(vectorHandle, name, [](void* ptr) noexcept {
        delete static_cast<Eigen::VectorXd*>(ptr);
        });

    size_t shape[1] = { static_cast<size_t>(vectorHandle->rows()) };
    return { vectorHandle->data(), 1, shape, owner };
}

LightOptParams& OptimizationParameters::params() const
{
    
    return ialt.getOptimizer().optimizationParametersByLightRef(ref);
}

void OptimizationParameters::reset() const
{
    params().reset();
}

void OptimizationParameters::setDefault() const
{
    params().setDefault();
}

void OptimizationParameters::setAll() const
{
    params().setAll();
}

bool OptimizationParameters::get(const LightOptParams::PARAMS type) const
{
    return params()[type];
}

void OptimizationParameters::set(const LightOptParams::PARAMS type, const bool value) const
{
    params()[type] = value;
}

std::string OptimizationParameters::toReprString() const
{
    std::stringstream stream;
    stream << "OptimizationParameters { ";

    auto& p = params();

    for (int i = 0; i != LightOptParams::MAX_PARAMS; i++) {
        if (i != 0) {
            stream << ", ";
        }

        const auto type = static_cast<LightOptParams::PARAMS>(i);
        stream << LightOptParams::name(type) << "=";
        stream << (p[type] ? "true" : "false");
    }

    stream << " }";
    return stream.str();
}

void OptimizationParametersLight::setRotations() const
{
    params().setRotations();
}

void OptimizationParametersLight::setPositions() const
{
    params().setPositions();
}

bool OptimizationParametersMesh::getEmissiveStrength() const
{
    return params()[LightOptParams::PARAMS::INTENSITY];
}

bool OptimizationParametersMesh::getEmissiveFactor() const
{
    return params()[LightOptParams::PARAMS::COLOR_R] || params()[LightOptParams::PARAMS::COLOR_G] || params()[LightOptParams::PARAMS::COLOR_B];
}

void OptimizationParametersMesh::setEmissiveStrength(const bool b) const
{
    params()[LightOptParams::PARAMS::INTENSITY] = b;
    IALT().checkCurrent().handle()->getOptimizer().exportLightSettings();
}

void OptimizationParametersMesh::setEmissiveFactor(const bool b) const
{
    params()[LightOptParams::PARAMS::COLOR_R] = b;
    params()[LightOptParams::PARAMS::COLOR_G] = b;
    params()[LightOptParams::PARAMS::COLOR_B] = b;
    IALT().checkCurrent().handle()->getOptimizer().exportLightSettings();
}



NB_MODULE(pyialt, m) {
    tamashii::registerBackend(std::make_shared<VulkanRenderBackendInteractiveAdjointLightTracing>());
    
    Exports& exports = CoreModule::exportCore(m);
    exportDefaultRasterizer(m, exports);
    exportDefaultPathTracer(m, exports);

    
    LightTraceOptimizer::vars::initVars();

    static std::optional<nb::class_<IALT>> ialt;
    exports.impls().def_prop_ro_static("ialt", [](nb::handle)->IALT { return {}; });
    

    ialt.emplace(m, "ialt")
        .def(nb::init())
        .def_static("switch", []()->IALT
        {
            Common::getInstance().changeBackendImplementation(name);
            return {};
        });


    ialt->def_prop_rw(
        "bounces",
        [](const IALT& self) { return self.handle()->getOptimizer().bounces(); },
        [](const IALT& self, const int value) { self.handle()->getOptimizer().bounces() = value; }
    );

    ialt->def("getFrameImage", [](const IALT& self) {
        Common::getInstance().frame();
        const auto img = self.handle()->getFrameImage();

        const auto vectorHandle = new std::vector{ std::move(img->getDataVector()) };
        nb::capsule owner(vectorHandle, "Screenshot capsule", [](void* ptr) noexcept {
            delete static_cast<std::vector<uint8_t>*>(ptr);
            });

        size_t shape[3] = { img->getWidth(), img->getHeight(), 4 };
        return python::Image<float>{ vectorHandle->data(), 3, shape, owner };
        });

    ialt->def("getTargetImage", [](const IALT& self) {
        self.handle()->showTarget(true);
        Common::getInstance().frame();
        const auto img = self.handle()->getFrameImage();
        self.handle()->showTarget(false);
        Common::getInstance().frame();

        const auto vectorHandle = new std::vector{ std::move(img->getDataVector()) };
        nb::capsule owner(vectorHandle, "Screenshot capsule", [](void* ptr) noexcept {
            delete static_cast<std::vector<uint8_t>*>(ptr);
            });

        size_t shape[3] = { img->getWidth(), img->getHeight(), 4 };
        return python::Image<float>{ vectorHandle->data(), 3, shape, owner };
        });

    nb::class_<python::OptimizationParameters> optParams{ m, "OptimizationParameters" };
    optParams
        .def("reset", &OptimizationParameters::reset)
        .def("setDefault", &OptimizationParameters::setDefault)
        .def("setAll", &OptimizationParameters::setAll)
        .def("__repr__", &OptimizationParameters::toReprString);

    for (int i = 0; i != LightOptParams::MAX_PARAMS; i++) {
        auto type = static_cast<LightOptParams::PARAMS>(i);
        optParams.def_prop_rw(
            LightOptParams::name(type),
            [type](const OptimizationParameters& params) { return params.get(type); },
            [type](const OptimizationParameters& params, const bool value) { params.set(type, value); }
        );
    }

    nb::class_<python::OptimizationParametersLight, OptimizationParameters> optParamsLight{ m, "OptimizationParametersLight" };
    optParamsLight
        .def("setRotations", &OptimizationParametersLight::setRotations)
        .def("setPositions", &OptimizationParametersLight::setPositions)
        .def("__repr__", &OptimizationParametersLight::toReprString);

    nb::class_<python::OptimizationParametersMesh, OptimizationParameters> optParamsMesh{ m, "OptimizationParametersMesh" };
    optParamsMesh
        .def_prop_rw("emissiveStrength", &OptimizationParametersMesh::getEmissiveStrength, &OptimizationParametersMesh::setEmissiveStrength)
        .def_prop_rw("emissiveFactor", &OptimizationParametersMesh::getEmissiveFactor, &OptimizationParametersMesh::setEmissiveFactor)
        .def("__repr__", &OptimizationParametersMesh::toReprString);

    nb::enum_<LightOptParams::PARAMS>(m, "Param")
        .value("X", LightOptParams::PARAMS::POS_X)
        .value("Y", LightOptParams::PARAMS::POS_Y)
        .value("Z", LightOptParams::PARAMS::POS_Z);
    exports.light().def("getGradImage", [](const python::Light& light, const LightOptParams::PARAMS param, const size_t spp) {
	    const auto ialt = IALT().checkCurrent().handle();
        auto& refLight = light.refLight();
        ialt->clearGradVis();
        ialt->showGradVis({ { const_cast<tamashii::RefLight*>(&refLight), param } });

        for (size_t i = 0; i < spp; i++) Common::getInstance().frame();
        ialt->showGradVis({});
        Common::getInstance().frame();

        const auto img = ialt->getGradImage();

        const auto ptr = reinterpret_cast<glm::vec4*>(img->getDataVector().data());
        for (size_t i = 0; i < (img->getWidth() * img->getHeight()); i++) ptr[i] /= spp;

        const auto vectorHandle = new std::vector{ std::move(img->getDataVector()) };
        nb::capsule owner(vectorHandle, "Screenshot capsule", [](void* ptr) noexcept {
            delete static_cast<std::vector<uint8_t>*>(ptr);
            });

        size_t shape[3] = { img->getWidth(), img->getHeight(), 4 };
        return python::Image<float>{ vectorHandle->data(), 3, shape, owner };
        });
    exports.light().def("getFDGradImage", [](const python::Light& light, const LightOptParams::PARAMS param, const size_t spp, const float h) {
	    const auto ialt = IALT().checkCurrent().handle();
        auto& refLight = light.refLight();
        ialt->clearGradVis();
        ialt->showFDGradVis({ { const_cast<tamashii::RefLight*>(&refLight), param } }, h);
        Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
        for (uint32_t i = 0; i < spp; i++) Common::getInstance().frame();
        ialt->showFDGradVis({}, h);
        Common::getInstance().frame();

        const auto img = ialt->getGradImage();

        const auto ptr = reinterpret_cast<glm::vec4*>(img->getDataVector().data());
        for (size_t i = 0; i < (img->getWidth() * img->getHeight()); i++) ptr[i] /= spp;

        const auto vectorHandle = new std::vector{ std::move(img->getDataVector()) };
        nb::capsule owner(vectorHandle, "Screenshot capsule", [](void* ptr) noexcept {
            delete static_cast<std::vector<uint8_t>*>(ptr);
            });

        size_t shape[3] = { img->getWidth(), img->getHeight(), 4 };
        return python::Image<float>{ vectorHandle->data(), 3, shape, owner };
        });

    nb::enum_<LightTraceOptimizer::Optimizers>(m, "Optimizers")
        .value("LBFGS", LightTraceOptimizer::Optimizers::LBFGS)
        .value("GD", LightTraceOptimizer::Optimizers::GD)
        .value("ADAM", LightTraceOptimizer::Optimizers::ADAM)
        .value("FD_FORWARD", LightTraceOptimizer::Optimizers::FD_FORWARD)
        .value("FD_CENTRAL", LightTraceOptimizer::Optimizers::FD_CENTRAL)
        .value("CMA_ES", LightTraceOptimizer::Optimizers::CMA_ES);

    exports.light()
        .def_prop_ro("optimize", [](const python::Light& light) {
	        auto& refLight = light.refLight();
	        return OptimizationParameters{ *IALT().checkCurrent().handle(), refLight};
    });

    exports.mesh()
        .def("setTarget", &setMeshTargetC)
        .def("setTarget", &setMeshTargetCW)
        .def_prop_ro("optimize", [](const python::Mesh& mesh) {
	        auto& refMesh = mesh.refMesh();
	        return OptimizationParametersMesh{ *IALT().checkCurrent().handle(), refMesh };
	    });
    exports.scene()
        .def("lightsToParameterVector", [](python::Scene& scene) {
	        Eigen::VectorXd paramVec;
            IALT().checkCurrent().handle()->getOptimizer().lightsToParameterVector(paramVec);

	        return eigenVectorToPythonVector(std::move(paramVec), "Light parameter capsule");
    });

    exports.scene()
	.def("parameterVectorToLights", [](python::Scene& scene, python::Array<double> vecArray) {
        if (vecArray.ndim() != 1) {
            CoreModule::exitWithError("Invalid parameter vector. Expected only a single dimension.");
        }

        
        Eigen::VectorXd vec =  Eigen::Map<Eigen::VectorXd>( vecArray.data(), static_cast<unsigned int>(vecArray.shape(0)) );
        IALT().checkCurrent().handle()->getOptimizer().parameterVectorToLights(vec);
    }, "parameterVector"_a)
    .def("setTarget", &setSceneTargetC)
	.def("setTarget", &setSceneTargetCW);

    ialt->def("forward", [](IALT& self, python::Array<double> vecArray) {
        if (vecArray.ndim() != 1) {
            CoreModule::exitWithError("Invalid parameter vector. Expected only a single dimension.");
        }

        const Eigen::Map<Eigen::VectorXd> mappedVec{ vecArray.data(), static_cast<unsigned int>(vecArray.shape(0)) };
        IALT().checkCurrent().handle()->runForward(mappedVec);
            }, "parameterVector"_a);

    ialt->def("backward", [](IALT& self) {
        Eigen::VectorXd derivParams;
        auto phi = IALT().checkCurrent().handle()->runBackward(derivParams);

        auto pythonVec = eigenVectorToPythonVector(std::move(derivParams), "Derivative parameter capsule");
        return std::tuple<double, python::Array<double>>{ phi, pythonVec };
            });

    ialt->def("currentRadianceAsTarget", [](IALT& self, const bool clearWeights) {
        IALT().checkCurrent().handle()->useCurrentRadianceAsTarget(clearWeights);
    }, "clearWeights"_a);

    ialt->def("optimize", [](IALT& self, LightTraceOptimizer::Optimizers optimizerType, const float stepSize, const int maxIterations) {
            auto result = IALT().checkCurrent().handle()->runOptimizer(optimizerType, stepSize, maxIterations);

            
            using PyHistory = std::vector<python::Array<double>>;
            PyHistory pyHistory;
            pyHistory.reserve(result.history.size());
            for (auto& p : result.history) {
                pyHistory.emplace_back( eigenVectorToPythonVector(std::move(p), "History Item") );
            }
            return std::tuple<PyHistory, double> { pyHistory, result.lastPhi };
        }, "optimizerType"_a, "stepSize"_a = 1.0, "maxIterations"_a = 100);

    CoreModule::exitCallback([]() {
        ialt.reset();
    });
}
