#pragma once
#include <tamashii/core/scene/render_scene.hpp>
#include <rvk/rvk.hpp>

T_BEGIN_NAMESPACE
class TextureDataVulkan {
public:
														TextureDataVulkan(rvk::LogicalDevice* aDevice);
														~TextureDataVulkan();

	void												prepare(uint32_t aDescShaderStages, uint32_t aMaxSamplers = 256);
	void												destroy();

	void												loadScene(rvk::SingleTimeCommand* aStc, const tamashii::SceneBackendData& aScene);
	void												loadScene(rvk::SingleTimeCommand* aStc, const std::deque<tamashii::Image*>& aImages, const std::deque<Texture*>& aTextures);

	void												unloadScene();
	void												update(rvk::SingleTimeCommand* aStc, tamashii::SceneBackendData aScene);
	void												update(rvk::SingleTimeCommand* aStc, const std::deque<tamashii::Image*>& aImages, const std::deque<Texture*>& aTextures);

														
	rvk::Descriptor*									getDescriptor();
														
	rvk::Image*											getImage(Image *aImg);
														
	int													getIndex(Texture *aTex);

private:
	rvk::LogicalDevice*									mDevice;
	uint32_t											mDescMaxSamplers;

	std::vector<rvk::Image*>							mImages;
	std::unordered_map<Image*, rvk::Image*>				mImgToRvkimg;	
	std::unordered_map<Texture*, uint32_t>				mTexToDescIdx;	
	rvk::Descriptor										mTexDescriptor;
};

class CubemapTextureData_GPU {
public:
														CubemapTextureData_GPU(rvk::LogicalDevice* aDevice);
														~CubemapTextureData_GPU();
	void												prepare(uint32_t aDescShaderStages, uint32_t aMaxSamplers = 4);
	void												destroy();

	void												loadScene(rvk::SingleTimeCommand* aStc, const std::vector<ImageBasedLight*>& aLights);
	void												unloadScene();

	rvk::Descriptor*									getDescriptor();
	rvk::Image*											getImage(ImageBasedLight* aLight);
	int													getIndex(ImageBasedLight* aLight);
private:
	rvk::LogicalDevice*									mDevice;
	uint32_t											mDescMaxSamplers;

	std::vector<rvk::Image*>							mImages;
	std::unordered_map<ImageBasedLight*, rvk::Image*>	mImgToRvkimg;
	std::unordered_map<ImageBasedLight*, uint32_t>		mTexToDescIdx;
	rvk::Descriptor										mTexDescriptor;
};

class BlueNoiseTextureData_GPU {
public:
														BlueNoiseTextureData_GPU(rvk::LogicalDevice* aDevice);
														~BlueNoiseTextureData_GPU() = default;
														
														
														
	void												init(rvk::SingleTimeCommand* aStc, const std::vector<std::string>& aImagePaths, uint32_t aChannels, glm::uvec2 aResolution, bool a16Bits = false);
	void												destroy();
	rvk::Image*											getImage();
private:
	rvk::LogicalDevice*									mDevice;
	rvk::Image											mImg;
};
T_END_NAMESPACE