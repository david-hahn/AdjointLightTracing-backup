#pragma once

#include "ialt.hpp"

#include <tamashii/implementations/default_rasterizer.hpp>
#include <tamashii/implementations/default_path_tracer.hpp>

class VulkanRenderBackendInteractiveAdjointLightTracing final : public tamashii::VulkanRenderBackend {
public:
	std::vector<tamashii::RenderBackendImplementation*> initImplementations(tamashii::VulkanInstance* aInstance) override
	{
		return {
			new tamashii::DefaultRasterizer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
			new tamashii::DefaultPathTracer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
			new InteractiveAdjointLightTracing{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }}
		};
	}
};
