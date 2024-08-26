
#include <tamashii/tamashii.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include "example_impl.hpp"

T_USE_NAMESPACE

int main(int argc, char* argv[]) {
	
	tamashii::registerBackend(std::make_shared<VulkanRenderBackendExampleCompute>());
	var::default_implementation.value(EXAMPLE_IMPLEMENTATION_NAME);
	
	init(argc, argv);

	ExampleComputeImpl::runComputation();

	shutdown();
}
