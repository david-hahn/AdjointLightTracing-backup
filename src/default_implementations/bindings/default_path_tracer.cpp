#include <tamashii/implementations/bindings/default_path_tracer.hpp>
#include <nanobind/nanobind.h>
#include <tamashii/core/common/common.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/core_module.hpp>
#include <tamashii/bindings/exports.hpp>
#include <tamashii/implementations/default_path_tracer.hpp>

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS

namespace {
    constexpr const char* name = "Path Tracer";
    class PathTracer
    {
    public:
        PathTracer()
        {
            CoreModule::initGuard();
            const auto impl = Common::getInstance().getRenderSystem()->findBackendImplementation(name);
            if (impl == nullptr) CoreModule::exitWithError("Expected \"{}\" to be a registered Impl", name);
            _impl = dynamic_cast<DefaultPathTracer*>(impl);
        }

        void checkCurrent() const
        {
            if (Common::getInstance().getRenderSystem()->getCurrentBackendImplementations() != _impl)
            {
                CoreModule::exitWithError("Expected \"{}\" to be current Impl", name);
            }
        }

        python::Image<float> render(const int spp, const bool srgb, const bool alpha) const
        {
            checkCurrent();
            _impl->accumulate(true);
            for (int i = 0; i < spp; i++) Common::getInstance().frame();
            _impl->accumulate(false);

            auto image = _impl->getAccumulationImage(srgb, alpha);

            _impl->clearAccumulationBuffers();

            
            const auto vectorHandle = new std::vector<uint8_t>{ std::move(image.getDataVector()) };
            nb::capsule owner(vectorHandle, "Path Tracer img capsule", [](void* ptr) noexcept {
                delete static_cast<std::vector<uint8_t>*>(ptr);
                });

            size_t shape[3] = { image.getHeight(), image.getWidth(), alpha ? 4u : 3u };
            return { vectorHandle->data(), 3, shape, owner };
        }

        bool accumulate() const
        {
            checkCurrent();
            return _impl->accumulate();
        }
        void accumulate(bool b) const
        {
            checkCurrent();
            _impl->accumulate(b);
        }

        [[nodiscard]] DefaultPathTracer* handle() const { return _impl; }
    private:
        DefaultPathTracer* _impl;
    };
}

void python::exportDefaultPathTracer(const nb::module_& m, Exports& exports)
{
	static std::optional<nb::class_<PathTracer>> pathTracer;
    if (pathTracer.has_value()) CoreModule::exitWithError("{} already exported", name);

    exports.impls().def_prop_ro_static("path_tracer", [](nb::handle)->PathTracer { return {}; });

    pathTracer.emplace(m, "path_tracer");

    nb::enum_<DefaultPathTracer::Integrator>(pathTracer.value(), "Integrator")
        .value("PathBrdf", DefaultPathTracer::Integrator::PathBRDF)
        .value("PathHalf", DefaultPathTracer::Integrator::PathHalf)
        .value("PathMis", DefaultPathTracer::Integrator::PathMIS);

    pathTracer.value()
        .def(nb::init())
        .def_static("switch", []()->PathTracer
        {
        	Common::getInstance().changeBackendImplementation(name);
	        return {};
        })
        .def("render", &PathTracer::render, "spp"_a, "srgb"_a = false, "alpha"_a = false)
        .def_prop_rw("accumulate",
            [](const PathTracer& self) { return self.accumulate(); },
            [](const PathTracer& self, const bool b) { self.accumulate(b); })
        .def_prop_rw("maxDepth", 
            [](const PathTracer& self) { return self.handle()->getDepth(); },
            [](const PathTracer& self, const int depth) { self.handle()->setDepth(depth); })
        .def_prop_rw("integrator",
            [](const PathTracer& self) { return self.handle()->getIntegrator(); },
            [](const PathTracer& self, const DefaultPathTracer::Integrator integrator) { self.handle()->setIntegrator(integrator); });

    CoreModule::exitCallback([]() {
        pathTracer.reset();
    });
}
