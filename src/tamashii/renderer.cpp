#include "renderer.hpp"
#include <tamashii/implementations/default_rasterizer.hpp>
#include <tamashii/implementations/default_path_tracer.hpp>
std::vector<tamashii::RenderBackendImplementation*> VulkanRenderBackendDefault::initImplementations(tamashii::VulkanInstance* aInstance)
{
	return {
		new tamashii::DefaultRasterizer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
		new tamashii::DefaultPathTracer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
	};
}
