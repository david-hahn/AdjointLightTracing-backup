#pragma once
#include <tamashii/core/scene/render_scene.hpp>
#include <tamashii/core/scene/material.hpp>
#include <rvk/rvk.hpp>

T_BEGIN_NAMESPACE
class TextureDataVulkan;
class MaterialDataVulkan {
public:
												MaterialDataVulkan(rvk::LogicalDevice* aDevice);
												~MaterialDataVulkan();

												
	void										prepare(uint32_t aMaterialBufferUsageFlags, uint32_t aMaterialCount = 128, bool aAutoResize = false);
	void										destroy();

	void										loadScene(rvk::SingleTimeCommand* aStc, SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan);
	void										loadScene(rvk::SingleTimeCommand* aStc, const std::deque<Material*>& aMaterials, TextureDataVulkan* aTextureDataVulkan);
	void										unloadScene();
	void										update(rvk::SingleTimeCommand* aStc, SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan);
	void										update(rvk::SingleTimeCommand* aStc, const std::deque<Material*>& aMaterials, TextureDataVulkan* aTextureDataVulkan);
	bool										bufferChanged(bool aReset = true);

	rvk::Buffer*								getMaterialBuffer();
	int											getIndex(Material* aMaterial);
private:
	rvk::LogicalDevice*							mDevice;
	uint32_t									mMaxMaterialCount;
	std::vector<Material_s>						mMaterials;
	std::unordered_map<Material*, uint32_t>		mMaterialToIndex;		

	bool										mAutoResize;
	bool										mBufferResized;
	rvk::Buffer									mMaterialBuffer;
	uint32_t									mMaterialBufferUsageFlags;
};

T_END_NAMESPACE