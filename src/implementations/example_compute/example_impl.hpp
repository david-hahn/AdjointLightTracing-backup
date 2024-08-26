#pragma once
#include <tamashii/core/render/render_backend_implementation.hpp>
#include <tamashii/renderer_vk/render_backend.hpp>
#include <rvk/rvk.hpp>

#define EXAMPLE_IMPLEMENTATION_NAME "Example Compute"

class ExampleComputeImpl final : public tamashii::RenderBackendImplementation {
public:
						ExampleComputeImpl(const tamashii::VulkanRenderRoot& aRoot);
						~ExampleComputeImpl() override = default;
						ExampleComputeImpl(const ExampleComputeImpl&) = delete;
						ExampleComputeImpl& operator=(const ExampleComputeImpl&) = delete;
						ExampleComputeImpl(ExampleComputeImpl&&) = delete;
						ExampleComputeImpl& operator=(ExampleComputeImpl&&) = delete;

	const char*			getName() override { return EXAMPLE_IMPLEMENTATION_NAME; }
	void				windowSizeChanged(uint32_t aWidth, uint32_t aHeight) override;

						
	void				prepare(tamashii::RenderInfo_s& aRenderInfo) override;
	void				destroy() override;

						
	void				sceneLoad(tamashii::SceneBackendData aScene) override;
	void				sceneUnload(tamashii::SceneBackendData aScene) override;

						
    void				drawView(tamashii::ViewDef_s* aViewDef) override;
    void				drawUI(tamashii::UiConf_s* aUiConf) override;

						
	void				doCompute();

	static void			runComputation();

private:
	tamashii::VulkanRenderRoot mRoot;
};


class VulkanRenderBackendExampleCompute final : public tamashii::VulkanRenderBackend {
public:
	std::vector<tamashii::RenderBackendImplementation*> initImplementations(tamashii::VulkanInstance* aInstance) override;
};
