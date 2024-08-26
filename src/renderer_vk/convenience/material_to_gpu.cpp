#include <tamashii/renderer_vk/convenience/material_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/texture_to_gpu.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tamashii/core/scene/light.hpp>

T_USE_NAMESPACE
MaterialDataVulkan::MaterialDataVulkan(rvk::LogicalDevice* aDevice) : mDevice(aDevice), mMaxMaterialCount(0),
                                                                      mAutoResize(false), mBufferResized(false),
                                                                      mMaterialBuffer(mDevice), mMaterialBufferUsageFlags(0)
{
}

MaterialDataVulkan::~MaterialDataVulkan()
{ destroy(); }

void MaterialDataVulkan::prepare(const uint32_t aMaterialBufferUsageFlags, const uint32_t aMaterialCount, const bool aAutoResize)
{
	mMaxMaterialCount = aMaterialCount;
	mMaterials.reserve(aMaterialCount);
	mMaterialToIndex.reserve(aMaterialCount);
	mMaterialBuffer.create(aMaterialBufferUsageFlags, aMaterialCount * sizeof(Material_s), rvk::Buffer::Location::DEVICE);
	mMaterialBufferUsageFlags = aMaterialBufferUsageFlags;
	mAutoResize = aAutoResize;
	mBufferResized = false;
}
void MaterialDataVulkan::destroy()
{
	unloadScene();
	mMaterialBuffer.destroy();
	mMaxMaterialCount = 0;
	mMaterialBufferUsageFlags = 0;
	mAutoResize = false;
}

void MaterialDataVulkan::loadScene(rvk::SingleTimeCommand* aStc, const SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan) {
	loadScene(aStc, aScene.materials, aTextureDataVulkan);
}
void MaterialDataVulkan::loadScene(rvk::SingleTimeCommand* aStc, const std::deque<Material*>& aMaterials, TextureDataVulkan* aTextureDataVulkan)
{
	unloadScene();

	if(mAutoResize && aMaterials.size() > mMaxMaterialCount)
	{
		spdlog::warn("Convenience: Material Buffer to small, resizing...");
		const uint32_t materialBufferUsageFlags = mMaterialBufferUsageFlags;
		destroy();
		prepare(materialBufferUsageFlags, static_cast<uint32_t>(aMaterials.size()));
		mBufferResized = true;
	}

	if (aMaterials.empty()) return;
	for (Material *material : aMaterials) {
		Material_s m = material->getRawData();
		m.baseColorTexIdx = aTextureDataVulkan->getIndex(material->getBaseColorTexture());
		m.metallicTexIdx = aTextureDataVulkan->getIndex(material->getMetallicTexture());
		m.roughnessTexIdx = aTextureDataVulkan->getIndex(material->getRoughnessTexture());
		m.occlusionTexIdx = aTextureDataVulkan->getIndex(material->getOcclusionTexture());
		m.normalTexIdx = aTextureDataVulkan->getIndex(material->getNormalTexture());
		m.emissionTexIdx = aTextureDataVulkan->getIndex(material->getEmissionTexture());

		m.specularTexIdx = aTextureDataVulkan->getIndex(material->getSpecularTexture());
		m.specularColorTexIdx = aTextureDataVulkan->getIndex(material->getSpecularColorTexture());
		m.transmissionTexIdx = aTextureDataVulkan->getIndex(material->getTransmissionTexture());
		m.lightTexIdx = aTextureDataVulkan->getIndex(material->getLightTexture());
		m.customTexIdx = aTextureDataVulkan->getIndex(material->getCustomTexture());
		mMaterialToIndex.insert(std::pair(material, static_cast<uint32_t>(mMaterials.size())));
		mMaterials.push_back(m);

		if (mMaterials.size() > mMaxMaterialCount) spdlog::error("Material count > buffer size");
	}
	mMaterialBuffer.STC_UploadData(aStc, mMaterials.data(), mMaterials.size() * sizeof(Material_s), 0);
}

void MaterialDataVulkan::unloadScene()
{
	mMaterials.clear();
	mMaterialToIndex.clear();
	mBufferResized = false;
}

void MaterialDataVulkan::update(rvk::SingleTimeCommand* aStc, const SceneBackendData aScene, TextureDataVulkan* aTextureDataVulkan) {
	update(aStc,aScene.materials, aTextureDataVulkan);
}
void MaterialDataVulkan::update(rvk::SingleTimeCommand* aStc, const const std::deque<Material*>& aMaterials, TextureDataVulkan* aTextureDataVulkan)
{
	unloadScene();
	loadScene(aStc, aMaterials, aTextureDataVulkan);
}

bool MaterialDataVulkan::bufferChanged(const bool aReset)
{
	const bool ret = mBufferResized;
	if (aReset) mBufferResized = false;
	return ret;
}

rvk::Buffer* MaterialDataVulkan::getMaterialBuffer()
{ return &mMaterialBuffer; }

int MaterialDataVulkan::getIndex(Material *aMaterial)
{
	if (aMaterial == nullptr || (mMaterialToIndex.find(aMaterial) == mMaterialToIndex.end())) return -1;
	else return static_cast<int>(mMaterialToIndex[aMaterial]);
}
