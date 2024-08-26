#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/scene/render_cmd_system.hpp>

T_BEGIN_NAMESPACE
class RenderBackendImplementation {
public:
					RenderBackendImplementation() = default;
	virtual			~RenderBackendImplementation() = default;

	virtual const char*	getName() = 0;

	virtual void	windowSizeChanged(const uint32_t aWidth, const uint32_t aHeight) {}
	virtual void	entityAdded(const Ref& aRef) {}
	virtual void	entityRemoved(const Ref& aRef) {}
					
					
	virtual bool	drawOnMesh(const DrawInfo* aDrawInfo) { return false; }
	virtual void	screenshot(const std::string& aFilename) { spdlog::warn("Impl-Screenshot function not implemented"); }

					
	virtual void	prepare(RenderInfo_s& aRenderInfo) = 0;
	virtual void	destroy() = 0;

					
	virtual void	sceneLoad(SceneBackendData aScene) = 0;
	virtual void	sceneUnload(SceneBackendData aScene) = 0;

					
	virtual void	drawView(ViewDef_s* aViewDef) = 0;
	virtual void	drawUI(UiConf_s* aUiConf) = 0;
};
T_END_NAMESPACE