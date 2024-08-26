#pragma once
#include <tamashii/core/scene/render_scene.hpp>
#include <tamashii/core/scene/light.hpp>
#include <rvk/rvk.hpp>

T_BEGIN_NAMESPACE
class TextureDataVulkan;
class GeometryDataVulkan;
class LightDataVulkan {
public:
												LightDataVulkan(rvk::LogicalDevice* aDevice);
												~LightDataVulkan();

												
	void										prepare(uint32_t aLightBufferUsageFlags, uint32_t aLightCount = 128);
	void										destroy();

	void										loadScene(rvk::SingleTimeCommand* aStc, SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan = nullptr, GeometryDataVulkan* aGeometryDataVulkan = nullptr);
	void										loadScene(rvk::SingleTimeCommand* aStc, const std::deque<std::shared_ptr<RefLight>>* aRefLights, TextureDataVulkan* aTextureDataVulkan = nullptr,
													const std::deque<std::shared_ptr<RefModel>>* aRefModels = nullptr, GeometryDataVulkan* aGeometryDataVulkan = nullptr);
	void										unloadScene();
	void										update(rvk::SingleTimeCommand* aStc, SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan = nullptr, GeometryDataVulkan* aGeometryDataVulkan = nullptr);
	void										update(rvk::SingleTimeCommand* aStc, const std::deque<std::shared_ptr<RefLight>>* aRefLights, TextureDataVulkan* aTextureDataVulkan = nullptr,
													const std::deque<std::shared_ptr<RefModel>>* aRefModels = nullptr, GeometryDataVulkan* aGeometryDataVulkan = nullptr);

	rvk::Buffer*								getLightBuffer();
	int											getIndex(RefLight* aRefLight);
	int											getIndex(RefMesh* aRefMesh);
	uint32_t									getLightCount() const;
private:
	rvk::LogicalDevice*							mDevice;
	uint32_t									mMaxLightCount;
	std::vector<Light_s>						mLights;
	std::unordered_map<RefLight*, uint32_t>	mRefLightToIndex;		
	std::unordered_map<RefMesh*, uint32_t>	mRefMeshToIndex;		
	rvk::Buffer									mLightBuffer;
};

T_END_NAMESPACE