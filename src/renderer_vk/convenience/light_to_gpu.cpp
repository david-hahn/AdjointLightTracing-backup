#include <tamashii/renderer_vk/convenience/light_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/texture_to_gpu.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tamashii/core/scene/ref_entities.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/material.hpp>

#include "tamashii/renderer_vk/convenience/geometry_to_gpu.hpp"

T_USE_NAMESPACE

LightDataVulkan::LightDataVulkan(rvk::LogicalDevice* aDevice): mDevice(aDevice), mMaxLightCount(0), mLightBuffer(aDevice)
{}

LightDataVulkan::~LightDataVulkan()
{ destroy(); }

void LightDataVulkan::prepare(const uint32_t aLightBufferUsageFlags, const uint32_t aLightCount)
{
	mMaxLightCount = aLightCount;
	mLights.reserve(aLightCount);
	mRefLightToIndex.reserve(aLightCount);
	mRefMeshToIndex.reserve(aLightCount);
	mLightBuffer.create(aLightBufferUsageFlags, aLightCount * sizeof(Light_s), rvk::Buffer::Location::DEVICE);
}
void LightDataVulkan::destroy()
{
	unloadScene();
	mLightBuffer.destroy();
	mMaxLightCount = 0;
}

void LightDataVulkan::loadScene(rvk::SingleTimeCommand* aStc, const SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan, GeometryDataVulkan* aGeometryDataVulkan)
{
	loadScene(aStc, &aScene.refLights, aTextureDataVulkan, &aScene.refModels, aGeometryDataVulkan);
}

void LightDataVulkan::loadScene(rvk::SingleTimeCommand* aStc, const std::deque<std::shared_ptr<RefLight>>* aRefLights, TextureDataVulkan* aTextureDataVulkan,
	const std::deque<std::shared_ptr<RefModel>>* aRefModels, GeometryDataVulkan* aGeometryDataVulkan)
{
	unloadScene();
	if (!aRefLights->empty()) {
		for (auto& refLight : *aRefLights) {
			Light_s l = refLight->light->getRawData();
			l.pos_ws = glm::vec4(refLight->position, 1);
			l.n_ws_norm = glm::vec4(refLight->direction,0);
			l.t_ws_norm = glm::normalize(refLight->model_matrix * refLight->light->getDefaultTangent());

			if (refLight->light->getType() == Light::Type::IES && aTextureDataVulkan)
			{
				l.texture_index = aTextureDataVulkan->getIndex(dynamic_cast<IESLight&>(*refLight->light).getCandelaTexture());
			}

			
			mRefLightToIndex.insert(std::pair(refLight.get(), static_cast<uint32_t>(mLights.size())));
			mLights.push_back(l);
			if (mLights.size() > mMaxLightCount) spdlog::error("Light count > buffer size");
		}
	}
	if (aTextureDataVulkan && aRefModels && aGeometryDataVulkan) {
		
		for (auto& refModel : *aRefModels) {
			Light_s l{};
			l.type = static_cast<uint32_t>(LightType::TRIANGLE_MESH);
			int geometryIndex = 0;
			for (const auto& refMesh : refModel->refMeshes) {
				if (!refMesh->mesh->getMaterial()->isLight()) continue;
				const auto material = refMesh->mesh->getMaterial();
				const auto tex = material->getEmissionTexture();
				const auto offsets = aGeometryDataVulkan->getOffset(refMesh->mesh.get());
				
				glm::mat4 mat = glm::transpose(refModel->model_matrix);
				l.pos_ws = mat[0];
				l.n_ws_norm = mat[1];
				l.t_ws_norm = mat[2];
				l.intensity = material->getEmissionStrength();
				l.color = material->getEmissionFactor();
				l.texture_index = tex ? aTextureDataVulkan->getIndex(tex) : -1;
				l.id = refModel->ref_model_index;
				l.double_sided = !refMesh->mesh->getMaterial()->getCullBackface();
				
				l.index_buffer_offset = refMesh->mesh->hasIndices() ? offsets.mIndexOffset : -1;
				l.vertex_buffer_offset = offsets.mVertexOffset;
				l.triangle_count = static_cast<uint32_t>(refMesh->mesh->getPrimitiveCount());

				mRefMeshToIndex.insert(std::pair(refMesh.get(), static_cast<uint32_t>(mLights.size())));
				mLights.push_back(l);
				if (mLights.size() > mMaxLightCount) spdlog::error("Light count > buffer size");
			}
		}
	}
	if(!mLights.empty()) mLightBuffer.STC_UploadData(aStc, mLights.data(), mLights.size() * sizeof(Light_s), 0);
}

void LightDataVulkan::unloadScene()
{
	mLights.clear();
	mRefLightToIndex.clear();
	mRefMeshToIndex.clear();
}

void LightDataVulkan::update(rvk::SingleTimeCommand* aStc, const SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan, GeometryDataVulkan* aGeometryDataVulkan)
{
	loadScene(aStc, &aScene.refLights, aTextureDataVulkan, &aScene.refModels, aGeometryDataVulkan);
}

void LightDataVulkan::update(rvk::SingleTimeCommand* aStc, const std::deque<std::shared_ptr<RefLight>>* aRefLights, TextureDataVulkan* aTextureDataVulkan, const std::deque<std::shared_ptr<RefModel>>* aRefModels, GeometryDataVulkan* aGeometryDataVulkan)
{
	unloadScene();
	loadScene(aStc, aRefLights, aTextureDataVulkan, aRefModels, aGeometryDataVulkan);
}

rvk::Buffer* LightDataVulkan::getLightBuffer()
{ return &mLightBuffer; }

uint32_t LightDataVulkan::getLightCount() const
{ return static_cast<uint32_t>(mLights.size()); }

int LightDataVulkan::getIndex(RefLight* const aRefLight)
{
	if (aRefLight == nullptr || (mRefLightToIndex.find(aRefLight) == mRefLightToIndex.end())) return -1;
	return static_cast<int>(mRefLightToIndex[aRefLight]);
}

int LightDataVulkan::getIndex(RefMesh* aRefMesh)
{
	if (aRefMesh == nullptr || (mRefMeshToIndex.find(aRefMesh) == mRefMeshToIndex.end())) return -1;
	return static_cast<int>(mRefMeshToIndex[aRefMesh]);
}
