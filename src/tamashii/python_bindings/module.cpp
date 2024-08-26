
#include <nanobind/nanobind.h>
#include <tamashii/tamashii.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/core_module.hpp>
#include <tamashii/bindings/exports.hpp>
#include <tamashii/implementations/bindings/default_rasterizer.hpp>
#include <tamashii/implementations/bindings/default_path_tracer.hpp>

#include "../renderer.hpp"

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS


NB_MODULE(pymashii, m) {
    tamashii::registerBackend(std::make_shared<VulkanRenderBackendDefault>());
    
    Exports& exports = CoreModule::exportCore(m);
    exportDefaultRasterizer(m, exports);
    exportDefaultPathTracer(m, exports);

    
    

	
    
	
	
}
