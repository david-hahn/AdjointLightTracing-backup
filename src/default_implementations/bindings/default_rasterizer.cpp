#include <tamashii/implementations/bindings/default_rasterizer.hpp>
#include <nanobind/nanobind.h>
#include <tamashii/core/common/common.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/core_module.hpp>
#include <tamashii/bindings/exports.hpp>
#include <tamashii/implementations/default_rasterizer.hpp>

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS

namespace {
    constexpr const char* name = "Rasterizer";
    class Rasterizer
    {
    public:
        Rasterizer()
        {
            CoreModule::initGuard();
            const auto impl = Common::getInstance().getRenderSystem()->findBackendImplementation(name);
            if (impl == nullptr) CoreModule::exitWithError("Expected \"{}\" to be a registered Impl", name);
            _impl = dynamic_cast<DefaultRasterizer*>(impl);
        }

        void checkCurrent() const
        {
            if (Common::getInstance().getRenderSystem()->getCurrentBackendImplementations() != _impl)
            {
                CoreModule::exitWithError("Expected \"{}\" to be current Impl", name);
            }
        }
        
        [[nodiscard]] DefaultRasterizer* handle() const { return _impl; }
    private:
        DefaultRasterizer* _impl;
    };
}

void python::exportDefaultRasterizer(const nb::module_& m, Exports& exports)
{
    static std::optional<nb::class_<Rasterizer>> rasterizer;
    if (rasterizer.has_value()) CoreModule::exitWithError( "{} already exported", name);

    exports.impls().def_prop_ro_static("rasterizer", [](nb::handle)->Rasterizer { return {}; });

    rasterizer.emplace(m, "rasterizer")
        .def(nb::init())
        .def_static("switch", []()->Rasterizer
        {
        	Common::getInstance().changeBackendImplementation(name);
	        return {};
        });

    CoreModule::exitCallback([]() {
        rasterizer.reset();
    });
}