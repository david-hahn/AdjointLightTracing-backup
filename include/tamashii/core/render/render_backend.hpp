#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>
#include <tamashii/core/scene/render_scene.hpp>

T_BEGIN_NAMESPACE
class RenderBackend {
public:
										RenderBackend() = default;
	virtual								~RenderBackend() = default;
	virtual const char*					getName() = 0;
										
										
	virtual void						init(Window* aMainWindow = nullptr) = 0;
	virtual void						registerMainWindow(Window* aMainWindow, bool aSizeChanged = true) = 0;
	virtual void						unregisterMainWindow(Window* aMainWindow) = 0;
	virtual void						shutdown() = 0;

	virtual void						addImplementation(RenderBackendImplementation* aImplementation) = 0;
	virtual void						reloadImplementation(SceneBackendData aScene) = 0;
	virtual void						changeImplementation(uint32_t aIndex, SceneBackendData aScene) = 0;
	virtual RenderBackendImplementation* getCurrentBackendImplementations() const = 0;
	virtual std::vector<RenderBackendImplementation*>& getAvailableBackendImplementations() = 0;

										
	virtual void						recreateRenderSurface(const uint32_t aWidth, const uint32_t aHeight) {}
	virtual void						entitiyAdded(const Ref& aRef) const {}
	virtual void						entitiyRemoved(const Ref& aRef) const {}
	virtual bool						drawOnMesh(const DrawInfo* aDrawInfo) const { return false; }
	virtual void						screenshot(const std::string& aName) const {}

										
										
	virtual void						sceneLoad(SceneBackendData aScene) = 0;
	virtual void						sceneUnload(SceneBackendData aScene) = 0;

										
	virtual void						beginFrame() = 0;
	virtual void						drawView(ViewDef_s* aViewDef) = 0;
	virtual void						drawUI(UiConf_s* aUiConf) = 0;
	virtual void						captureSwapchain(ScreenshotInfo_s* aScreenshotInfo) {}
	virtual void						endFrame() = 0;
private:
	
	virtual void						prepare() const = 0;
	virtual void						destroy() const = 0;
};
T_END_NAMESPACE