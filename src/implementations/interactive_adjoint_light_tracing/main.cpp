
#include <tamashii/tamashii.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/renderer_vk/render_backend.hpp>

#include "renderer.hpp"

int main(int argc, char* argv[]) {
	
	
	

	
	tamashii::registerBackend(std::make_shared<VulkanRenderBackendInteractiveAdjointLightTracing>());
	LightTraceOptimizer::vars::initVars();

	
	tamashii::run(argc, argv);
}
