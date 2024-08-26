
#include <tamashii/tamashii.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/render/render_backend_implementation.hpp>
#include <tamashii/renderer_vk/render_backend.hpp>

class MyImpl final : public tamashii::RenderBackendImplementation {
public:
					MyImpl() = default;
					~MyImpl() override = default;
					MyImpl(const MyImpl&) = delete;
					MyImpl& operator=(const MyImpl&) = delete;
					MyImpl(MyImpl&&) = delete;
					MyImpl& operator=(MyImpl&&) = delete;

	const char*		getName() override { return "MyImpl"; }

	void			windowSizeChanged(uint32_t aWidth, uint32_t aHeight) override;

	
	void			prepare(tamashii::RenderInfo_s& aRenderInfo) override;
	void			destroy() override;

	
	void			sceneLoad(tamashii::SceneBackendData aScene) override;
	void			sceneUnload(tamashii::SceneBackendData aScene) override;

	
	void			drawView(tamashii::ViewDef_s* aViewDef) override;
	void			drawUI(tamashii::UiConf_s* aUiConf) override;
};

void MyImpl::windowSizeChanged(const uint32_t aWidth, const uint32_t aHeight) {}


void MyImpl::prepare(tamashii::RenderInfo_s& aRenderInfo) {}
void MyImpl::destroy() {}

void MyImpl::sceneLoad(tamashii::SceneBackendData aScene) {}
void MyImpl::sceneUnload(tamashii::SceneBackendData aScene) {}

void MyImpl::drawView(tamashii::ViewDef_s* aViewDef) {}
void MyImpl::drawUI(tamashii::UiConf_s* aUiConf) {}

#include <tamashii/implementations/default_rasterizer.hpp>
#include <tamashii/implementations/default_path_tracer.hpp>
class VulkanRenderBackendEmpty final : public tamashii::VulkanRenderBackend {
public:
	std::vector<tamashii::RenderBackendImplementation*> initImplementations(tamashii::VulkanInstance* aInstance) override
	{
		return {
			new tamashii::DefaultRasterizer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
			new tamashii::DefaultPathTracer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
			new MyImpl{}
		};
	}
};

int main(int argc, char* argv[]) {
	
	tamashii::registerBackend(std::make_shared<VulkanRenderBackendEmpty>());
	
	tamashii::run(argc, argv);
}
