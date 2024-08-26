#include <tamashii/core/scene/render_scene.hpp>

#include <tamashii/core/scene/scene_graph.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/camera.hpp>
#include <tamashii/core/scene/material.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tamashii/core/scene/ref_entities.hpp>
#include <tamashii/core/scene/render_cmd_system.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/platform/system.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>


T_USE_NAMESPACE

RenderScene::RenderScene() : mReady{ false }, mSceneGraph{ nullptr }, mCurrentCamera{ nullptr }, mSelection{}, mPlayAnimation{ false }, mAnimationCycleTime{ 0 }, mAnimationTime{ 0 }, mUpdateRequests{}
{
	
	mDefaultCamera = std::make_shared<Camera>();
	mDefaultCamera->initPerspectiveCamera(glm::radians(45.0f), 1.0f, 0.1f, 10000.0f);
	mDefaultCamera->setName(DEFAULT_CAMERA_NAME);

	mDefaultCameraRef = std::make_shared<RefCameraPrivate>();
	mDefaultCameraRef->mode = RefCamera::Mode::EDITOR;
	mDefaultCameraRef->camera = mDefaultCamera;
	dynamic_cast<RefCameraPrivate&>(*mDefaultCameraRef).default_camera = true;
	mDefaultCameraRef->ref_camera_index = static_cast<int>(mRefCameras.size());
	mRefCameras.emplace_back(mDefaultCameraRef);

	mCurrentCamera = mRefCameras.front();
	for (const auto& rc : mRefCameras) {
		if (rc->camera->getName() == var::default_camera.value()) {
			mCurrentCamera = rc;
			break;
		}
	}
	mSceneGraph = Node::alloc("Root");
}

RenderScene::~RenderScene()
{
	destroy();
}

bool RenderScene::initFromFile(const std::string& aFile)
{
	mSceneFile = aFile;

	const auto si = io::Import::instance().load_scene(aFile);
	if (!si) return false;
	return initFromData(*si);
}

bool RenderScene::initFromData(io::SceneData& aSceneInfo)
{
	destroy();
	filterSceneInfo(aSceneInfo);

	mAnimationCycleTime = aSceneInfo.mCycleTime;
	mSceneGraph = aSceneInfo.mSceneGraphs.front();
	mModels = aSceneInfo.mModels;
	mMaterials = aSceneInfo.mMaterials;
	mTextures = aSceneInfo.mTextures;
	mImages = aSceneInfo.mImages;
	mCameras = aSceneInfo.mCameras;
	mLights = aSceneInfo.mLights;

	for (auto& n : *mSceneGraph) {
		traverseSceneGraph(*n);
	}

    mCurrentCamera = mRefCameras.front();
	for (const auto& rc : mRefCameras) {
		if (rc->camera->getName() == var::default_camera.value()) mCurrentCamera = rc;
	}
	return true;
}

bool RenderScene::addSceneFromFile(const std::string& aFile)
{
	const auto si = io::Import::instance().load_scene(aFile);
	if (!si) return false;
	return addSceneFromData(*si);
}

bool RenderScene::addSceneFromData(io::SceneData& aSceneInfo)
{
	filterSceneInfo(aSceneInfo);

	mAnimationCycleTime = std::max(mAnimationCycleTime, aSceneInfo.mCycleTime);
	mModels.insert(mModels.end(), aSceneInfo.mModels.begin(), aSceneInfo.mModels.end());
	mMaterials.insert(mMaterials.end(), aSceneInfo.mMaterials.begin(), aSceneInfo.mMaterials.end());
	mTextures.insert(mTextures.end(), aSceneInfo.mTextures.begin(), aSceneInfo.mTextures.end());
	mImages.insert(mImages.end(), aSceneInfo.mImages.begin(), aSceneInfo.mImages.end());
	mCameras.insert(mCameras.end(), aSceneInfo.mCameras.begin(), aSceneInfo.mCameras.end());
	mLights.insert(mLights.end(), aSceneInfo.mLights.begin(), aSceneInfo.mLights.end());

	mSceneGraph = aSceneInfo.mSceneGraphs.front();
	traverseSceneGraph(*mSceneGraph);

	for (const auto& rc : mRefCameras) {
		if (rc->camera->getName() == var::default_camera.value()) mCurrentCamera = rc;
	}
	return true;
}

void RenderScene::destroy()
{
	mReady.store(false);
	mSelection = {};
	mNewlyAddedRef.clear();
	mNewlyRemovedRef.clear();
	mNewlyRemovedAsset.clear();

	
	for (const Image* img : mImages) delete img;
	/*for (Light* l : mLights) {
		switch (l->getType()) {
		case Light::Type::POINT: delete static_cast<PointLight*>(l); break;
		case Light::Type::SPOT: delete static_cast<SpotLight*>(l); break;
		case Light::Type::DIRECTIONAL: delete static_cast<DirectionalLight*>(l); break;
		case Light::Type::IES: delete static_cast<IESLight*>(l); break;
		case Light::Type::SURFACE: delete static_cast<SurfaceLight*>(l); break;
		}
	}*/
	for (const Material* m : mMaterials) delete m;
	for (const Texture* t : mTextures) delete t;
	mSceneGraph.reset();
	mModels.clear();
	mImages.clear();
	mCameras.clear();
	mLights.clear();
	mMaterials.clear();
	mSceneGraph = nullptr;

	mCurrentCamera = nullptr;
	
	
	mRefModels.clear();
	mRefLights.clear();
	mRefCameras.clear();

	
	mRefCameras.push_back(mDefaultCameraRef);
}

void RenderScene::readyToRender(const bool aReady)
{ mReady.store(aReady); }

bool RenderScene::readyToRender() const
{ return mReady.load(); }

std::shared_ptr<RefLight> RenderScene::addLightRef(const std::shared_ptr<Light>& aLight, const glm::vec3 aPosition, const glm::vec4 aRotation, const glm::vec3 aScale)
{
	mLights.push_back(aLight);

	Node& node = mSceneGraph->addChildNode("refLight");
	node.setTranslation(aPosition);
	node.setRotation(aRotation);
	node.setScale(aScale);
	node.setLight(aLight);

	auto refLight = std::make_shared<RefLight>();
	refLight->light = node.getLight();
	refLight->ref_light_index = static_cast<int>(mRefLights.size());
	refLight->transforms.push_back(&node.getTRS());
	refLight->model_matrix *= node.getTRS().getMatrix(std::fmod(mAnimationTime, mAnimationCycleTime));
	const glm::vec4 dir = refLight->model_matrix * refLight->light->getDefaultDirection();
	const glm::vec4 pos = refLight->model_matrix * glm::vec4(0, 0, 0, 1);
	refLight->direction = glm::normalize(glm::vec3(dir));
	refLight->position = glm::vec3(pos);
	mRefLights.emplace_back(refLight);

	mSelection.reference = refLight;
	mNewlyAddedRef.emplace_back(refLight);
	requestLightUpdate();

	if (refLight->light->getType() == Light::Type::IES)
	{
		const auto& ies = dynamic_cast<IESLight&>(*refLight->light);
		mTextures.push_back(ies.getCandelaTexture());
		mImages.push_back(ies.getCandelaTexture()->image);
		requestImageUpdate();
		requestTextureUpdate();
	}

	return refLight;
}

void RenderScene::addMaterial(Material* aMaterial)
{
	if (std::find(mMaterials.begin(), mMaterials.end(), aMaterial) == mMaterials.end()) mMaterials.push_back(aMaterial);
}

std::shared_ptr<RefModel> RenderScene::addModelRef(const std::shared_ptr<Model>& aModel, const glm::vec3 aPosition, const glm::vec4 aRotation, const glm::vec3 aScale)
{
	mModels.push_back(aModel);
	
	Node& node = mSceneGraph->addChildNode("refModel");
	node.setTranslation(aPosition);
	node.setRotation(aRotation);
	node.setScale(aScale);
	node.setModel(aModel);

	auto refModel = std::make_shared<RefModel>();
	refModel->model = node.getModel();
	refModel->ref_model_index = static_cast<int>(mRefModels.size());
	refModel->transforms.push_back(&node.getTRS());
	refModel->model_matrix *= node.getTRS().getMatrix(std::fmod(mAnimationTime, mAnimationCycleTime));
	for (const auto& me : *refModel->model) {
		addMaterial(me->getMaterial());
		auto refMesh = std::make_shared<RefMesh>();
		refMesh->mesh = me;
		refModel->refMeshes.push_back(refMesh);
	}
	mRefModels.emplace_back(refModel);

	mSelection.reference = refModel;
	mNewlyAddedRef.emplace_back(refModel);
	requestMaterialUpdate();
	requestModelGeometryUpdate();
	requestModelInstanceUpdate();

	return refModel;
}

void RenderScene::removeModel(std::shared_ptr<RefModel> const& aRefModel)
{
	const auto it = std::find(mRefModels.begin(), mRefModels.end(), aRefModel);
	if (it != mRefModels.end())
	{
		mRefModels.erase(it);
		mNewlyRemovedRef.push_back(aRefModel);
		for (uint32_t i = 0; i < mRefModels.size(); i++) mRefModels[i]->ref_model_index = static_cast<int>(i);

		
		const auto* modelPtr = aRefModel->model.get();
		uint32_t count = 0;
		for (const auto& refModel : mRefModels) if (modelPtr == refModel->model.get()) count++;
		if(count == 0) {
			const auto it2 = std::find(mModels.begin(), mModels.end(), aRefModel->model);
			if (it2 != mModels.end()) {
				mNewlyRemovedAsset.push_back(aRefModel->model);
				mModels.erase(it2);
			}
		}

		mUpdateRequests.mModelInstances = true;
		mUpdateRequests.mModelGeometries = true;
		for (const auto mesh : *aRefModel->model) mUpdateRequests.mLights |= mesh->getMaterial()->isLight();
	}
}

void RenderScene::removeLight(const std::shared_ptr<RefLight>& aRefLight)
{
	const auto it = std::find(mRefLights.begin(), mRefLights.end(), aRefLight);
	if (it != mRefLights.end())
	{
		mRefLights.erase(it);
		mNewlyRemovedRef.push_back(aRefLight);
		for (uint32_t i = 0; i < mRefLights.size(); i++) mRefLights[i]->ref_light_index = static_cast<int>(i);

		
		const auto* lightPtr = aRefLight->light.get();
		uint32_t count = 0;
		for (const auto& refLight : mRefLights) if (lightPtr == refLight->light.get()) count++;
		if (count == 0) {
			const auto it2 = std::find(mLights.begin(), mLights.end(), aRefLight->light);
			if (it2 != mLights.end()) {
				mNewlyRemovedAsset.push_back(aRefLight->light);
				mLights.erase(it2);
			}
		}

		mUpdateRequests.mLights = true;
	}
}

void RenderScene::requestImageUpdate()
{ mUpdateRequests.mImages = true; }

void RenderScene::requestTextureUpdate()
{ mUpdateRequests.mTextures = true; }

void RenderScene::requestMaterialUpdate()
{ mUpdateRequests.mMaterials = true; }

void RenderScene::requestModelInstanceUpdate()
{ mUpdateRequests.mModelInstances = true; }

void RenderScene::requestModelGeometryUpdate()
{ mUpdateRequests.mModelGeometries = true; }

void RenderScene::requestLightUpdate()
{ mUpdateRequests.mLights = true; }

void RenderScene::requestCameraUpdate()
{ mUpdateRequests.mCamera = true; }

void RenderScene::intersect(const glm::vec3 aOrigin, const glm::vec3 aDirection, const IntersectionSettings aSettings, Intersection *aHitInfo) const
{
	float t = std::numeric_limits<float>::max();
	auto barycentric = glm::vec2(0.0f);
	aHitInfo->mTmin = std::numeric_limits<float>::max();
	if (aSettings.mHitMask == HitMask::All || aSettings.mHitMask == HitMask::Geometry) {
		for (auto& refModel : mRefModels) {
			const glm::mat4 inverseModel = glm::inverse(refModel->model_matrix);
			const glm::vec3 osOrigin = glm::vec3(inverseModel * glm::vec4(aOrigin, 1));
			const glm::vec3 osDirection = glm::vec3(inverseModel * glm::vec4(aDirection, 0));
			if (refModel->model->getAABB().intersect(osOrigin, osDirection, t)) {
				uint32_t meshIndex = 0;
				for (const auto& refMesh : refModel->refMeshes) {
					if (refMesh->mesh->getAABB().intersect(osOrigin, osDirection, t)) {
						for (uint32_t i = 0; i < refMesh->mesh->getPrimitiveCount(); i++) {
							if (refMesh->mesh->getTriangle(i).intersect(osOrigin, osDirection, t, barycentric, refMesh->mesh->getMaterial()->getCullBackface() ? aSettings.mCullMode : CullMode::None)) {
								if (t < aHitInfo->mTmin) {
									aHitInfo->mHit = refModel;
									aHitInfo->mTmin = t;
									aHitInfo->mBarycentric = barycentric;
									aHitInfo->mRefMeshHit = refMesh.get();
									aHitInfo->mMeshIndex = meshIndex;
									aHitInfo->mPrimitiveIndex = i;
								}
							}
						}
					}
					meshIndex++;
				}
			}
			if (aHitInfo->mHit && aHitInfo->mHit->type == Ref::Type::Model) aHitInfo->mHitPos = refModel->model_matrix * glm::vec4((osOrigin + (osDirection * aHitInfo->mTmin)), 1);
		}
	}

	if (aSettings.mHitMask == HitMask::All || aSettings.mHitMask == HitMask::Light) {
		for (auto& refLight : mRefLights) {
			const glm::mat4 inverseModel = glm::inverse(refLight->model_matrix);
			const glm::vec3 osOrigin = glm::vec3(inverseModel * glm::vec4(aOrigin, 1));
			const glm::vec3 osDirection = glm::vec3(inverseModel * glm::vec4(aDirection, 0));
			if (refLight->light->getType() == Light::Type::SURFACE) {
				const auto& sl = dynamic_cast<SurfaceLight&>(*refLight->light);
				if (sl.getShape() == SurfaceLight::Shape::SQUARE || sl.getShape() == SurfaceLight::Shape::RECTANGLE) {
					aabb_s aabb = { {-0.5,-0.5,0}, {0.5,0.5,0} };
					if (aabb.intersect(osOrigin, osDirection, t)) {
						if (t < aHitInfo->mTmin) {
							aHitInfo->mHit = refLight;
							aHitInfo->mTmin = t;
						}
					}
				}
				else if (sl.getShape() == SurfaceLight::Shape::DISK || sl.getShape() == SurfaceLight::Shape::ELLIPSE) {
					disk_s d{};
					d.mCenter = sl.getCenter();
					d.mRadius = 0.5f;
					d.mNormal = sl.getDefaultDirection();
					if (d.intersect(osOrigin, osDirection, t)) {
						if (t < aHitInfo->mTmin) {
							aHitInfo->mHit = refLight;
							aHitInfo->mTmin = t;
						}
					}
				}
			}
			else {
				disk_s d{};
				d.mCenter = glm::vec3(0, 0, 0);
				d.mRadius = LIGHT_OVERLAY_RADIUS;
				d.mNormal = glm::normalize(-osDirection);
				if (d.intersect(osOrigin, osDirection, t)) {
					if (t < aHitInfo->mTmin) {
						aHitInfo->mHit = refLight;
						aHitInfo->mTmin = t;
					}
				}
			}
			if (aHitInfo->mHit && aHitInfo->mHit->type == Ref::Type::Light) aHitInfo->mHitPos = refLight->model_matrix * glm::vec4((osOrigin + (osDirection * aHitInfo->mTmin)), 1);
		}
	}
	if (aHitInfo->mHit) aHitInfo->mTmin = glm::length(aOrigin - aHitInfo->mHitPos);
}

bool RenderScene::animation() const
{ return mPlayAnimation; }

void RenderScene::setAnimation(const bool aPlay)
{ mPlayAnimation = aPlay; }

void RenderScene::resetAnimation()
{
	mAnimationTime = 0;
	update(0);
}

void RenderScene::setSelection(const Selection& aSelection)
{
	mSelection = aSelection;
}

void RenderScene::update(const float aMilliseconds)
{
	if (!mReady.load()) return;
	mAnimationTime += (aMilliseconds / 1000.0f);
	
	
	const float relativeTime = std::fmod(mAnimationTime, mAnimationCycleTime);
	
	for (const auto& refModel : mRefModels) {
		
		if (refModel->animated) {
			mUpdateRequests.mModelInstances |= true;
			refModel->model_matrix = glm::mat4(1.0f);
			for (const TRS *trs : refModel->transforms) {
				refModel->model_matrix *= trs->getMatrix(relativeTime);
			}
		}
	}
	
	for (const auto &refLight : mRefLights) {
		
		if (refLight->animated) {
			mUpdateRequests.mLights |= true;
			refLight->model_matrix = glm::mat4(1);
			for (const TRS *trs : refLight->transforms) {
				refLight->model_matrix *= trs->getMatrix(relativeTime);
			}
			refLight->direction = glm::normalize(glm::vec3(refLight->model_matrix * refLight->light->getDefaultDirection()));
			refLight->position = glm::vec3(refLight->model_matrix * glm::vec4(0, 0, 0, 1));
		}
	}
	
	for (auto& refCamera : mRefCameras) {
		if (dynamic_cast<RefCameraPrivate&>(*refCamera).default_camera) continue;
		if (refCamera->animated) {
			mUpdateRequests.mCamera |= true;
			refCamera->model_matrix = glm::mat4(1);
			for (const TRS *trs : refCamera->transforms) {
				refCamera->model_matrix *= trs->getMatrix(relativeTime);
			}
			dynamic_cast<RefCameraPrivate&>(*refCamera).setModelMatrix(refCamera->model_matrix, true);
		}
	}
}

void RenderScene::draw()
{
	if (!mReady.load()) return;

	RefCamera& refCam = *mCurrentCamera;
	Camera& cam = *refCam.camera;

	auto* vd = new ViewDef_s{ getSceneData(), Frustum(&refCam) };
	vd->updates = mUpdateRequests;
	mUpdateRequests = {};

	
	
	vd->projection_matrix = cam.getProjectionMatrix();
	vd->inv_projection_matrix = glm::inverse(vd->projection_matrix);
	
	vd->view_matrix = refCam.view_matrix;
	vd->inv_view_matrix = glm::inverse(refCam.view_matrix);
	
	vd->view_pos = getCurrentCameraPosition();
	vd->view_dir = getCurrentCameraDirection();

	
	
	for (auto& refModel : mRefModels) {
		
		
		const aabb_s aabb_model = refModel->model->getAABB().transform(refModel->model_matrix);
		if (!vd->view_frustum.checkAABBInside(aabb_model.mMin, aabb_model.mMax)) continue;
		
		vd->ref_models.push_back(refModel.get());
		for (const auto& refMesh : refModel->refMeshes) {
			if (refModel->refMeshes.size() != 1) {
				const aabb_s aabb_mesh = refMesh->mesh->getAABB().transform(refModel->model_matrix);
				if (!vd->view_frustum.checkAABBInside(aabb_mesh.mMin, aabb_mesh.mMax)) continue;
			}

			DrawSurf_s ds = {};
			ds.refMesh = refMesh.get();
			ds.model_matrix = &refModel->model_matrix;
			ds.ref_model_index = refModel->ref_model_index;
			
			ds.ref_model_ptr = refModel.get();
			vd->surfaces.push_back(ds);
		}
	}
	
	
	for (auto &refLight : mRefLights) {
		
		
		vd->lights.push_back(refLight.get());
	}
	
	for (const auto& r : mNewlyAddedRef) renderCmdSystem.addEntityAddedCmd(r);
	mNewlyAddedRef.clear();
	for (const auto& r : mNewlyRemovedRef) renderCmdSystem.addEntityRemovedCmd(r);
	mNewlyRemovedRef.clear();
	for (const auto& a : mNewlyRemovedAsset) renderCmdSystem.addAssetRemovedCmd(a);
	mNewlyRemovedAsset.clear();
	
	renderCmdSystem.addDrawSurfCmd(vd);
}

std::string RenderScene::getSceneFileName()
{ return mSceneFile; }

std::deque<std::shared_ptr<RefCamera>>& RenderScene::getAvailableCameras()
{ return mRefCameras; }

RefCamera& RenderScene::getCurrentCamera() const
{
	
	if (mCurrentCamera) { return *mCurrentCamera; }
	else return *mDefaultCameraRef;
}


std::shared_ptr<RefCamera> RenderScene::referenceCurrentCamera() const {
	return mCurrentCamera ? mCurrentCamera : mDefaultCameraRef;
}

void RenderScene::setCurrentCamera(std::shared_ptr<RefCamera> const& aCamera)
{
	if (mCurrentCamera != aCamera) {
		mCurrentCamera = aCamera;
		requestCameraUpdate();
	}
}

glm::vec3 RenderScene::getCurrentCameraPosition() const
{
	if (!mReady.load()) return glm::vec3(0);
	return mCurrentCamera->getPosition();
}

glm::vec3 RenderScene::getCurrentCameraDirection() const
{
	if (!mReady.load()) return glm::vec3(0);
	return mCurrentCamera->getDirection();
}

Selection& RenderScene::getSelection()
{
	return mSelection;
}

std::deque<std::shared_ptr<RefModel>>& RenderScene::getModelList()
{
	return mRefModels;
}

std::deque<std::shared_ptr<RefLight>>& RenderScene::getLightList()
{ return mRefLights; }

std::deque<std::shared_ptr<RefCamera>>& RenderScene::getCameraList()
{
	return mRefCameras;
}

float RenderScene::getCurrentTime() const
{ return mAnimationTime; }

float RenderScene::getCycleTime() const
{ return mAnimationCycleTime; }

SceneBackendData RenderScene::getSceneData()
{ return { mImages, mTextures, mModels, mMaterials, mRefModels, mRefLights, mRefCameras }; }

io::SceneData RenderScene::getSceneInfo()
{
	
	for (const auto& refLight : mRefLights) refLight->updateSceneGraphNodesFromModelMatrix();
	for (const auto& refCamera : mRefCameras) refCamera->updateSceneGraphNodesFromModelMatrix(refCamera->y_flipped);
	for (const auto& refModel : mRefModels) refModel->updateSceneGraphNodesFromModelMatrix();

	io::SceneData si = {};
	si.mCycleTime = mAnimationCycleTime;
	si.mSceneGraphs = { mSceneGraph };
	si.mModels = mModels;
	si.mCameras = mCameras;
	si.mLights = mLights;
	si.mMaterials = mMaterials;
	si.mTextures = mTextures;
	si.mImages = mImages;
	return si;
}

void RenderScene::filterSceneInfo(io::SceneData& aSceneInfo)
{
	if (var::unique_model_refs.value()) {
		std::unordered_map<Model*, bool> map(aSceneInfo.mModels.size());
		aSceneInfo.mModels.clear();
		const std::function f = [&map, &aSceneInfo](Node* aNode)
		{
			if (!aNode->hasModel()) return;
			auto& m = aNode->getModel();
			if (map.find(m.get()) == map.end()) {
				map.insert({ m.get(), true});
				aSceneInfo.mModels.emplace_back(m);
			}
			else {
				auto copiedModel = std::make_shared<Model>(*aNode->getModel());
				aSceneInfo.mModels.emplace_back(copiedModel);
				aNode->setModel(copiedModel);
			}
		};
		aSceneInfo.mSceneGraphs.front()->visit(f);
	}
	if (var::unique_material_refs.value()) {
		const std::deque<Material*> materialsOld = aSceneInfo.mMaterials;
		aSceneInfo.mMaterials.clear();
		for (const auto& model : aSceneInfo.mModels) {
			for (const auto& mesh : model->getMeshList()) {
				aSceneInfo.mMaterials.push_back(new Material(*mesh->getMaterial()));
				mesh->setMaterial(aSceneInfo.mMaterials.back());
			}
		}
		for (const Material* m : materialsOld) delete m;
	}
	if (var::unique_light_refs.value()) {
		std::unordered_map<Light*, bool> map(aSceneInfo.mLights.size());
		aSceneInfo.mLights.clear();
		const std::function f = [&map, &aSceneInfo](Node* aNode)
		{
			if (!aNode->hasLight()) return;
			auto& l = aNode->getLight();
			
			if (map.find(l.get()) == map.end()) {
				map.insert({ l.get(), true});
				aSceneInfo.mLights.emplace_back(l);
			}
			else {
				std::shared_ptr<Light> copiedLight;
				switch(l->getType())
				{
				case Light::Type::DIRECTIONAL: copiedLight = std::make_shared<DirectionalLight>(dynamic_cast<DirectionalLight&>(*l)); break;
				case Light::Type::POINT:copiedLight = std::make_shared<PointLight>(dynamic_cast<PointLight&>(*l)); break;
				case Light::Type::SPOT: copiedLight = std::make_shared<SpotLight>(dynamic_cast<SpotLight&>(*l));  break;
				case Light::Type::SURFACE: copiedLight = std::make_shared<SurfaceLight>(dynamic_cast<SurfaceLight&>(*l)); break;
				
				case Light::Type::IES: copiedLight = std::make_shared<IESLight>(dynamic_cast<IESLight&>(*l)); break;
				}
				aSceneInfo.mLights.emplace_back(copiedLight);
				aNode->setLight(copiedLight);
			}
		};
		aSceneInfo.mSceneGraphs.front()->visit(f);
	}
}

void RenderScene::traverseSceneGraph(Node& aNode, glm::mat4 aMatrix, const bool aAnimatedPath) {
	const bool animated_node = aAnimatedPath || aNode.hasAnimation();
	
	static std::deque<Node*> nodeHistory;
	nodeHistory.push_back(&aNode);
	
	static std::deque<TRS*> trsHistory;
	if (aNode.hasLocalTransform()) {
		trsHistory.push_back(&aNode.getTRS());
		
		aMatrix *= trsHistory.back()->getMatrix(std::fmod(mAnimationTime, mAnimationCycleTime));
	}
	
	if (aNode.hasModel()) {
		auto refModel = std::make_shared<RefModel>();
		refModel->model = aNode.getModel();
		refModel->ref_model_index = static_cast<int>(mRefModels.size());
		refModel->model_matrix = aMatrix;
		refModel->animated = animated_node;
		refModel->transforms = trsHistory;
		for (const auto& me : *refModel->model) {
			auto refMesh = std::make_shared<RefMesh>();
			refMesh->mesh = me;
			refModel->refMeshes.push_back(refMesh);
		}
		mRefModels.emplace_back(std::move(refModel));
	}
	
	if (aNode.hasCamera()) {
		auto refCamera = std::make_shared<RefCameraPrivate>();
		refCamera->setModelMatrix(aMatrix, true);
		refCamera->camera = aNode.getCamera();
		refCamera->ref_camera_index = static_cast<int>(mRefCameras.size());
		refCamera->animated = animated_node;
		refCamera->transforms = trsHistory;
		mRefCameras.emplace_back(std::move(refCamera));
	}
	
	if (aNode.hasLight()) {
		auto refLight = std::make_shared<RefLight>();
		refLight->light = aNode.getLight();
		refLight->ref_light_index = static_cast<int>(mRefLights.size());
		const glm::vec4 dir = aMatrix * refLight->light->getDefaultDirection();
		const glm::vec4 pos = aMatrix * glm::vec4(0, 0, 0, 1);
		refLight->direction = glm::normalize(glm::vec3(dir));
		refLight->position = glm::vec3(pos);
		refLight->model_matrix = aMatrix;
		refLight->animated = animated_node;
		refLight->transforms = trsHistory;
		mRefLights.emplace_back(std::move(refLight));
	}
	
	for (auto& n : aNode) {
		traverseSceneGraph(*n, aMatrix, animated_node);
	}
	
	if (aNode.hasLocalTransform()) trsHistory.pop_back();
	
	nodeHistory.pop_back();
}

