#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/scene/render_scene.hpp>
#include <tamashii/core/scene/ref_entities.hpp>

#include <deque>
#include <variant>

T_BEGIN_NAMESPACE
struct DrawSurf_s {
	RefMesh*					refMesh;			
	glm::mat4*					model_matrix;
	int							ref_model_index;	
	RefModel*					ref_model_ptr;		
};

struct RenderInfo_s
{
	bool						headless;
	uint32_t					frameCount;
	glm::ivec2					targetSize;
};

struct ViewDef_s {
	SceneBackendData						scene;
	Frustum						view_frustum;
	SceneUpdateInfo				updates;			
	glm::mat4					projection_matrix;
	glm::mat4					view_matrix;
	glm::mat4					inv_projection_matrix;
	glm::mat4					inv_view_matrix;

	glm::vec3					view_pos;
	glm::vec3					view_dir;

	uint32_t					frame_index;
	bool						headless;
	glm::ivec2					target_size;
	
	std::deque<DrawSurf_s>		surfaces;
	std::deque<RefModel*>		ref_models;
	std::deque<RefLight*>		lights;
};

struct UiConf_s {
	
	RenderScene*				scene;
	RenderSystem*				system;
	
	DrawInfo*					draw_info;
};

struct ScreenshotInfo_s {
	std::string					name;
	uint32_t					width;
	uint32_t					height;
	uint32_t					channels;
	std::vector<uint8_t>		data;
};

enum class RenderCommand
{
	EMPTY,
	BEGIN_FRAME,
	END_FRAME,
	DRAW_VIEW,
	DRAW_UI,
	ENTITY_ADDED,
	ENTITY_REMOVED,
	ASSET_ADDED,
	ASSET_REMOVED,
	SCREENSHOT,
	IMPL_SCREENSHOT,
	IMPL_DRAW_ON_MESH
};

struct RCmd_s final
{
	RenderCommand				mType;
	std::variant<
		ViewDef_s*,					
		UiConf_s*,					
		ScreenshotInfo_s*,			
		std::shared_ptr<Ref>,		
		std::shared_ptr<Asset>,		
		DrawInfo*					
	>							mValue;

private:
	void release();
	void copyConstruct(const RCmd_s& r);
};

class RenderCmdSystem {
public:

	void					addBeginFrameCmd();
	void					addEndFrameCmd();
    void					addDrawSurfCmd(ViewDef_s* aViewDef);
    void					addDrawUICmd(UiConf_s* aUiConf);
	void					addScreenshotCmd(std::string_view aFileName);
	void					addImplScreenshotCmd();
	void					addEntityAddedCmd(std::shared_ptr<Ref> aRef);
	void					addEntityRemovedCmd(std::shared_ptr<Ref> aRef);
	void					addAssetAddedCmd(std::shared_ptr<Asset> aAsset);
	void					addAssetRemovedCmd(std::shared_ptr<Asset> aAsset);
	void					addDrawOnMeshCmd(const DrawInfo* aDrawInfo, const Intersection* aHitInfo);

	bool					nextCmd();
	RCmd_s					popNextCmd();
	void					deleteCmd(const RCmd_s& aCmd);

	uint32_t				frames() const;
private:
	void					addCmd(const RCmd_s& aCmd);

	std::deque<RCmd_s>		mCmdList;
	std::mutex				mMutex;
	std::atomic_uint32_t	mFrames;
};
extern RenderCmdSystem renderCmdSystem;
T_END_NAMESPACE
