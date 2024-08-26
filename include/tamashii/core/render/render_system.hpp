#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/scene/render_scene.hpp>

#include <list>
#include <memory>

T_BEGIN_NAMESPACE
struct RenderConfig_s {
	
	uint32_t			frame_index;
	
	float				framerate_smooth;
	float				frametime_smooth;
	
	float				frametime; /* in milliseconds */

	bool				show_gui;
	bool				mark_lights;
};

class RenderSystem {
public:
										RenderSystem();
										~RenderSystem();

	
	bool								hasBackend() const { return !mRenderBackends.empty(); }

	void								init(tamashii::Window* aWindow = nullptr);
	void								registerMainWindow(tamashii::Window* aWindow) const;
	void								unregisterMainWindow(tamashii::Window* aWindow) const;
	void								shutdown();

	RenderConfig_s&						getConfig();
	bool								isInit() const;

	void								renderSurfaceResize(uint32_t aWidth, uint32_t aHeight) const;
	bool								drawOnMesh(const DrawInfo* aDrawInfo) const;

	void								processCommands();
	
	const std::shared_ptr<RenderScene>& getMainScene() const;
	std::shared_ptr<RenderScene>		allocRenderScene();
	void								freeMainRenderScene();
	void								removeRenderScene(const std::shared_ptr<RenderScene>&);
	void								setMainRenderScene(std::shared_ptr<RenderScene> aRenderScene);
	
	void								sceneLoad(const SceneBackendData& aScene) const;
	void								sceneUnload(const SceneBackendData& aScene) const;
	void								reloadCurrentScene() const;

	void								captureSwapchain(ScreenshotInfo_s* aScreenshotInfo) const;

	
	void								addImplementation(RenderBackendImplementation* aImplementation) const;
	void								reloadBackendImplementation() const;
	void								changeBackendImplementation(int aIndex) const;
	bool								changeBackendImplementation(const char* name) const;

	std::vector<RenderBackendImplementation*>& getAvailableBackendImplementations() const;
	RenderBackendImplementation*		getCurrentBackendImplementations() const;
	RenderBackendImplementation*		findBackendImplementation(const char* name) const;

										static std::deque<std::shared_ptr<RenderBackend>> mRegisteredBackends;
private:

	void								updateStatistics();

	bool								mInit;
	RenderConfig_s						mRenderConfig;

	
	std::list<std::shared_ptr<RenderScene>>	mScenes;
	std::shared_ptr<RenderScene>			mMainScene;


	
	std::vector<std::shared_ptr<RenderBackend>>	mRenderBackends;
	std::shared_ptr<RenderBackend>				mCurrendRenderBackend;
};
T_END_NAMESPACE
