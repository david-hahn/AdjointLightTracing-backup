#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>

#include <string>
#include <deque>

T_BEGIN_NAMESPACE


struct SceneBackendData
{
	
	std::deque<Image*>&							images;
	std::deque<Texture*>&						textures;
	std::deque<std::shared_ptr<Model>>&			models;
	std::deque<Material*>&						materials;
	
	
	std::deque<std::shared_ptr<RefModel>>&		refModels;
	std::deque<std::shared_ptr<RefLight>>&		refLights;
	std::deque<std::shared_ptr<RefCamera>>&		refCameras;
};
enum class HitMask {
	Geometry = 1,
	Light = 2,
	All = Geometry | Light
};
enum class CullMode {
	None = 0,
	Front = 1,
	Back = 2,
	Both = 3
};
struct IntersectionSettings
{
	IntersectionSettings() : mCullMode{ CullMode::None }, mHitMask{ HitMask::All } {}
	IntersectionSettings(const CullMode	aCullMode, const HitMask aHitMask) : mCullMode{ aCullMode }, mHitMask{ aHitMask } {}
	CullMode								mCullMode;
	HitMask									mHitMask;
};
struct Intersection {
	std::shared_ptr<Ref>					mHit;				
	RefMesh*								mRefMeshHit;		
	uint32_t								mMeshIndex;			
	uint32_t								mPrimitiveIndex;	
	glm::vec3								mOriginPos;			
	glm::vec3								mHitPos;			
	glm::vec2								mBarycentric;		
	float									mTmin;				
};
struct DrawInfo {
	enum class Target : uint32_t {
		VERTEX_COLOR,
		CUSTOM
	};
	Target									mTarget;
	glm::vec3								mCursorColor;
	bool									mDrawMode;			
	bool									mHoverOver;			
	Intersection							mHitInfo;
	glm::vec3								mPositionWs;		
	glm::vec3								mNormalWsNorm;		
    glm::vec3								mTangentWsNorm;		
	float									mRadius;			
	glm::vec4								mColor0;			
	glm::vec4								mColor1;			
	bool									mDrawRgb;
	bool									mDrawAlpha;
	bool									mSoftBrush;
	bool									mDrawAll;
};
struct Selection
{
	std::shared_ptr<Ref>	reference;
	uint32_t				meshOffset;
	uint32_t				primitiveOffset;
	uint32_t				vertexOffset;
};

struct SceneUpdateInfo
{
	bool									mImages;
	bool									mTextures;
	bool									mMaterials;
	bool									mModelInstances;	
	bool									mModelGeometries;	
	bool									mLights;
	bool									mCamera;

	[[nodiscard]] bool any() const { return mImages || mTextures || mMaterials || mModelInstances || mModelGeometries || mLights || mCamera; }
	[[nodiscard]] bool all() const { return mImages && mTextures && mMaterials && mModelInstances && mModelGeometries && mLights && mCamera; }
	SceneUpdateInfo operator| (const SceneUpdateInfo& aSceneUpdates) const
	{
		SceneUpdateInfo su = {};
		su.mImages = mImages || aSceneUpdates.mImages;
		su.mTextures = mTextures || aSceneUpdates.mTextures;
		su.mMaterials = mMaterials || aSceneUpdates.mMaterials;
		su.mModelInstances = mModelInstances || aSceneUpdates.mModelInstances;
		su.mModelGeometries = mModelGeometries || aSceneUpdates.mModelGeometries;
		su.mLights = mLights || aSceneUpdates.mLights;
		su.mCamera = mCamera || aSceneUpdates.mCamera;
		return su;
	}
	SceneUpdateInfo operator& (const SceneUpdateInfo& aSceneUpdates) const
	{
		SceneUpdateInfo su = {};
		su.mImages = mImages && aSceneUpdates.mImages;
		su.mTextures = mTextures && aSceneUpdates.mTextures;
		su.mMaterials = mMaterials && aSceneUpdates.mMaterials;
		su.mModelInstances = mModelInstances && aSceneUpdates.mModelInstances;
		su.mModelGeometries = mModelGeometries && aSceneUpdates.mModelGeometries;
		su.mLights = mLights && aSceneUpdates.mLights;
		su.mCamera = mCamera && aSceneUpdates.mCamera;
		return su;
	}
};

class RenderScene final {
public:
											RenderScene();
											~RenderScene();

	bool									initFromFile(const std::string& aFile);
	bool									initFromData(io::SceneData& aSceneInfo);
	bool									addSceneFromFile(const std::string& aFile);
	bool									addSceneFromData(io::SceneData& aSceneInfo);

	void									destroy();

											
											
	void									readyToRender(bool aReady);
	bool									readyToRender() const;

	std::shared_ptr<RefLight>				addLightRef(const std::shared_ptr<Light>& aLight, glm::vec3 aPosition = glm::vec3(0), glm::vec4 aRotation = glm::vec4(0), glm::vec3 aScale = glm::vec3(1));
	void									addMaterial(Material* aMaterial);
	std::shared_ptr<RefModel>				addModelRef(const std::shared_ptr<Model>& aModel, glm::vec3 aPosition = glm::vec3(0), glm::vec4 aRotation = glm::vec4(0), glm::vec3 aScale = glm::vec3(1));

	void									removeModel(const std::shared_ptr<RefModel>& aRefModel);
	void									removeLight(const std::shared_ptr<RefLight>& aRefLight);

											
	void									requestImageUpdate();
	void									requestTextureUpdate();
	void									requestMaterialUpdate();
	void									requestModelInstanceUpdate();
	void									requestModelGeometryUpdate();
	void									requestLightUpdate();
	void									requestCameraUpdate();

	void									intersect(glm::vec3 aOrigin, glm::vec3 aDirection, IntersectionSettings aSettings, Intersection *aHitInfo) const;

	bool									animation() const;
	void									setAnimation(bool aPlay);
	void									resetAnimation();

	void									setSelection(const Selection& aSelection);

	void									update(float aMilliseconds);
	void									draw();

	std::string								getSceneFileName();

	std::deque<std::shared_ptr<RefCamera>>& getAvailableCameras();
	RefCamera&								getCurrentCamera() const;
	std::shared_ptr<RefCamera>				referenceCurrentCamera() const;
	void									setCurrentCamera(const std::shared_ptr<RefCamera>& aCamera);
	glm::vec3								getCurrentCameraPosition() const;
	glm::vec3								getCurrentCameraDirection() const;
	Selection&						getSelection();

	std::deque<std::shared_ptr<RefModel>>& getModelList();
	std::deque<std::shared_ptr<RefLight>>& getLightList();
	std::deque<std::shared_ptr<RefCamera>>& getCameraList();

											
	float									getCurrentTime() const;
	float									getCycleTime() const;

	SceneBackendData						getSceneData();
	io::SceneData							getSceneInfo();
private:
	static void								filterSceneInfo(io::SceneData& aSceneInfo);
	void									traverseSceneGraph(Node& aNode, glm::mat4 aMatrix = glm::mat4(1.0f), bool aAnimatedPath = false);

	std::string								mSceneFile;
	std::atomic<bool>						mReady;	

											
	std::shared_ptr<Node>					mSceneGraph;
	std::deque<std::shared_ptr<Model>>		mModels;
	std::deque<std::shared_ptr<Camera>>		mCameras;
	std::deque<std::shared_ptr<Light>>		mLights;
	std::deque<Material*>					mMaterials;
	std::deque<Texture*>					mTextures;
	std::deque<Image*>						mImages;

											
	std::deque<std::shared_ptr<RefModel>>	mRefModels;
	std::deque<std::shared_ptr<RefCamera>>	mRefCameras;
	std::deque<std::shared_ptr<RefLight>>	mRefLights;

	std::shared_ptr<Camera>					mDefaultCamera;
	std::shared_ptr<RefCamera>				mDefaultCameraRef;

	std::shared_ptr<RefCamera>				mCurrentCamera;
	Selection							mSelection;

	bool									mPlayAnimation;
											
	float									mAnimationCycleTime;
											
	float									mAnimationTime;
											
	SceneUpdateInfo							mUpdateRequests;
											
	std::deque<std::shared_ptr<Ref>>		mNewlyAddedRef;
	std::deque<std::shared_ptr<Ref>>		mNewlyRemovedRef;
	std::deque<std::shared_ptr<Asset>>		mNewlyRemovedAsset;
};

T_END_NAMESPACE
