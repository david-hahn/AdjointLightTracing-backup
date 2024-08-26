#include <tamashii/renderer_vk/convenience/texture_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/rvk_type_converter.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/light.hpp>

#include <algorithm>

T_USE_NAMESPACE

TextureDataVulkan::TextureDataVulkan(rvk::LogicalDevice* aDevice) : mDevice(aDevice), mDescMaxSamplers(0), mTexDescriptor(aDevice)
{}

TextureDataVulkan::~TextureDataVulkan()
{ destroy(); }

void TextureDataVulkan::prepare(const uint32_t aDescShaderStages, const uint32_t aMaxSamplers)
{
	mDescMaxSamplers = std::min(aMaxSamplers, mDevice->getPhysicalDevice()->getProperties().limits.maxPerStageDescriptorSamplers);

	mImgToRvkimg.reserve(mDescMaxSamplers);
	mTexToDescIdx.reserve(mDescMaxSamplers);
	mImages.reserve(mDescMaxSamplers);

	mTexDescriptor.reserve(1);
	mTexDescriptor.addImageSampler(0, aDescShaderStages, mDescMaxSamplers);
	mTexDescriptor.setBindingFlags(0, (rvk::Descriptor::BindingFlag::PARTIALLY_BOUND | rvk::Descriptor::BindingFlag::VARIABLE_DESCRIPTOR_COUNT));
	mTexDescriptor.finish(false);
}

void TextureDataVulkan::destroy()
{
	unloadScene();
	mTexDescriptor.destroy();
}

void TextureDataVulkan::loadScene(rvk::SingleTimeCommand* aStc, const tamashii::SceneBackendData& aScene) {
	loadScene(aStc, aScene.images, aScene.textures);
}
void TextureDataVulkan::loadScene(rvk::SingleTimeCommand* aStc, const std::deque<tamashii::Image*>& aImages, const std::deque<Texture*>& aTextures)
{
	unloadScene();
	
	
	for (Image *img : aImages) {
		const VkFormat format = converter::imageFormatToRvkFormat(img->getFormat());
		uint32_t mipmapLevel = 1;
		if(img->needsMipMaps()) mipmapLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(img->getWidth(), img->getHeight())))) + 1;

		auto rvkImg = new rvk::Image(mDevice);
		const bool notInit = !rvkImg->isInit();
		ASSERT(notInit, "rvk::image exist");
		rvkImg->createImage2D(img->getWidth(), img->getHeight(), format,
			rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::SAMPLED, mipmapLevel);

		rvkImg->STC_UploadData2D(aStc, img->getWidth(), img->getHeight(), img->getPixelSizeInBytes(), img->getData());
		if(mipmapLevel > 1) rvkImg->STC_GenerateMipmaps(aStc);
		rvkImg->STC_TransitionImage(aStc, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		mImages.push_back(rvkImg);
		mImgToRvkimg.insert(std::pair<Image*, rvk::Image*>(img, rvkImg));

		if (mImages.size() > mDescMaxSamplers) spdlog::error("Image count > descriptor size");
	}
	
	uint32_t idx = 0;
	for (Texture *tex : aTextures) {
		rvk::Image *img = mImgToRvkimg[tex->image];
		img->setSampler(converter::samplerToRvkSampler(tex->sampler));
		mTexDescriptor.setImage(0, img, idx);

		mTexToDescIdx.insert(std::pair<Texture*, uint32_t>(tex, idx));
		idx++;
	}
	
	if (!aTextures.empty()) {
		mTexDescriptor.setUpdateSize(0, aTextures.size());
		mTexDescriptor.update();
	}
}

void TextureDataVulkan::unloadScene()
{
	mImgToRvkimg.clear();
	mTexToDescIdx.clear();

	for (const rvk::Image *img : mImages) {
		delete img;
	}
	mImages.clear();
}

void TextureDataVulkan::update(rvk::SingleTimeCommand* aStc, const tamashii::SceneBackendData aScene) {
	update(aStc, aScene.images, aScene.textures);
}
void TextureDataVulkan::update(rvk::SingleTimeCommand* aStc, const std::deque<tamashii::Image*>& aImages, const std::deque<Texture*>& aTextures)
{
	unloadScene();
	loadScene(aStc, aImages, aTextures);
}


rvk::Descriptor* TextureDataVulkan::getDescriptor()
{ return &mTexDescriptor; }

rvk::Image* TextureDataVulkan::getImage(Image *aImg)
{
	if (aImg == nullptr || (mImgToRvkimg.find(aImg) == mImgToRvkimg.end())) return nullptr;
	else return mImgToRvkimg[aImg];
}

int TextureDataVulkan::getIndex(Texture *aTex)
{
	if (aTex == nullptr || (mTexToDescIdx.find(aTex) == mTexToDescIdx.end())) return -1;
	else return static_cast<int>(mTexToDescIdx[aTex]);
}

CubemapTextureData_GPU::CubemapTextureData_GPU(rvk::LogicalDevice* aDevice) : mDevice(aDevice), mTexDescriptor(aDevice) {
}

CubemapTextureData_GPU::~CubemapTextureData_GPU() {
	destroy();
}

void CubemapTextureData_GPU::prepare(uint32_t aDescShaderStages, uint32_t aMaxSamplers)
{
	mDescMaxSamplers = std::min(aMaxSamplers, mDevice->getPhysicalDevice()->getProperties().limits.maxPerStageDescriptorSamplers);

	mImgToRvkimg.reserve(mDescMaxSamplers);
	mTexToDescIdx.reserve(mDescMaxSamplers);
	mImages.reserve(mDescMaxSamplers);

	mTexDescriptor.reserve(1);
	mTexDescriptor.addImageSampler(0, aDescShaderStages, mDescMaxSamplers);
	mTexDescriptor.setBindingFlags(0, (rvk::Descriptor::BindingFlag::PARTIALLY_BOUND | rvk::Descriptor::BindingFlag::VARIABLE_DESCRIPTOR_COUNT));
	mTexDescriptor.finish(false);
}

void CubemapTextureData_GPU::destroy()
{
	unloadScene();
	mTexDescriptor.destroy();
}

void CubemapTextureData_GPU::loadScene(rvk::SingleTimeCommand* aStc, const std::vector<ImageBasedLight*>& aLights)
{
	unloadScene();
	
	
	for (ImageBasedLight* light : aLights) {
		auto cubemap = light->getCubeMap();
		auto img = cubemap.front();
		const VkFormat format = converter::imageFormatToRvkFormat(img->getFormat());
		uint32_t mipmapLevel = 1;
		

		auto rvkImg = new rvk::Image(mDevice);
		const bool notInit = !rvkImg->isInit();
		ASSERT(notInit, "rvk::image exist");
		rvkImg->createCubeImage(img->getWidth(), img->getHeight(), format,
			rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::SAMPLED, mipmapLevel);

		uint32_t index = 0;
		for(auto cimg : cubemap) {
			rvkImg->STC_UploadData2D(aStc, cimg->getWidth(), cimg->getHeight(), cimg->getPixelSizeInBytes(), cimg->getData(), 0, index);
			
			index++;
		}
		rvkImg->STC_TransitionImage(aStc, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		mImages.push_back(rvkImg);
		mImgToRvkimg.insert(std::pair<ImageBasedLight*, rvk::Image*>(light, rvkImg));

		if (mImages.size() > mDescMaxSamplers) spdlog::error("Image count > descriptor size");
	}
	
	uint32_t idx = 0;
	for (ImageBasedLight* light : aLights) {
		rvk::Image* img = mImgToRvkimg[light];
		img->setSampler(converter::samplerToRvkSampler(light->getSampler()));
		mTexDescriptor.setImage(0, img, idx);

		mTexToDescIdx.insert(std::pair<ImageBasedLight*, uint32_t>(light, idx));
		idx++;
	}
	
	if (!aLights.empty()) {
		mTexDescriptor.setUpdateSize(0, aLights.size());
		mTexDescriptor.update();
	}
}

void CubemapTextureData_GPU::unloadScene()
{
	mImgToRvkimg.clear();
	mTexToDescIdx.clear();

	for (const rvk::Image* img : mImages) {
		delete img;
	}
	mImages.clear();
}

rvk::Descriptor* CubemapTextureData_GPU::getDescriptor()
{
	return &mTexDescriptor;
}

rvk::Image* CubemapTextureData_GPU::getImage(ImageBasedLight* aLight)
{
	if (aLight == nullptr || (mImgToRvkimg.find(aLight) == mImgToRvkimg.end())) return nullptr;
	else return mImgToRvkimg[aLight];
}

int CubemapTextureData_GPU::getIndex(ImageBasedLight* aLight)
{
	if (aLight == nullptr || (mTexToDescIdx.find(aLight) == mTexToDescIdx.end())) return -1;
	else return static_cast<int>(mTexToDescIdx[aLight]);
}

BlueNoiseTextureData_GPU::BlueNoiseTextureData_GPU(rvk::LogicalDevice* aDevice) : mDevice(aDevice), mImg(aDevice)
{}

void BlueNoiseTextureData_GPU::init(rvk::SingleTimeCommand* aStc, const std::vector<std::string>& aImagePaths, const uint32_t aChannels, const glm::uvec2 aResolution, const bool a16Bits)
{
	mImg.destroy();

	VkFormat format;
	int bytesPerPixelChannel;
	if (a16Bits) {
		format = VK_FORMAT_R16_UNORM;
		if (aChannels == 2) format = VK_FORMAT_R16G16_UNORM;
		else if (aChannels == 3) format = VK_FORMAT_R16G16B16_UNORM;
		else if (aChannels == 4) format = VK_FORMAT_R16G16B16A16_UNORM;
		bytesPerPixelChannel = 2;
	}
	else {
		format = VK_FORMAT_R8_UNORM;
		if (aChannels == 2) format = VK_FORMAT_R8G8_UNORM;
		else if (aChannels == 3) format = VK_FORMAT_R8G8B8_UNORM;
		else if (aChannels == 4) format = VK_FORMAT_R8G8B8A8_UNORM;
		bytesPerPixelChannel = 1;
	}

	
	mImg.createImage2D(aResolution.x, aResolution.y, format, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1, static_cast<uint32_t>(aImagePaths.size()));
	const size_t imgSize = static_cast<size_t>(aResolution.x) * static_cast<size_t>(aResolution.y);
	const uint32_t bytesPerPixel = aChannels * bytesPerPixelChannel;
	const size_t totalSize = imgSize * bytesPerPixel;
	
	std::vector<uint8_t> buffer(totalSize);
	for (uint32_t i = 0; i < aImagePaths.size(); i++) {
		const std::string& path = aImagePaths[i];
		Image* image = nullptr;
		if (a16Bits) image = tamashii::io::Import::load_image_16_bit(path);
		else image = tamashii::io::Import::load_image_8_bit(path);
		const uint8_t* imgData = image->getData();
		mImg.STC_UploadData2D(aStc, aResolution.x, aResolution.y, bytesPerPixel, &imgData[0], 0, i);
	}
}

void BlueNoiseTextureData_GPU::destroy()
{
	mImg.destroy();
}

rvk::Image* BlueNoiseTextureData_GPU::getImage()
{ return &mImg; }
