
#include <nanobind/nanobind.h>
#include <tamashii/tamashii.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/core_module.hpp>
#include <tamashii/bindings/exports.hpp>

#include "../example_impl.hpp"

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS


NB_MODULE(pycompute, m) {
    
    tamashii::registerBackend(std::make_shared<VulkanRenderBackendExampleCompute>());
    CoreModule::exportCore(m);

    
    var::default_implementation.value("Example Compute");

    
    m.def("runComputation", []() { CoreModule::initGuard(); ExampleComputeImpl::runComputation(); });
}
