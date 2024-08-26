#pragma once
#include <tamashii/renderer_vk/render_backend.hpp>

class VulkanRenderBackendDefault final : public tamashii::VulkanRenderBackend {
public:
	std::vector<tamashii::RenderBackendImplementation*> initImplementations(tamashii::VulkanInstance* aInstance) override;
};
