#include <tamashii/renderer_vk/convenience/rvk_type_converter.hpp>

T_USE_NAMESPACE

namespace {
	VkFilter textureWrapToVkSamplerFilter(const Sampler::Filter aFilter) {
		switch (aFilter) {
		case Sampler::Filter::NEAREST: return VK_FILTER_NEAREST;
		case Sampler::Filter::LINEAR: return VK_FILTER_LINEAR;
		default: return VK_FILTER_LINEAR;
		}
	};
	VkSamplerMipmapMode textureWrapToVkSamplerMipmapMode(const Sampler::Filter aFilter) {
		switch (aFilter) {
		case Sampler::Filter::NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case Sampler::Filter::LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
	};
	VkSamplerAddressMode textureWrapToVkSamplerAddressMode(const Sampler::Wrap aWrap) {
		switch (aWrap) {
		case Sampler::Wrap::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case Sampler::Wrap::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case Sampler::Wrap::CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case Sampler::Wrap::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	};
}
rvk::SamplerConfig converter::samplerToRvkSampler(const Sampler& aSampler) {
	rvk::SamplerConfig c;
	c.min = textureWrapToVkSamplerFilter(aSampler.min);
	c.mag = textureWrapToVkSamplerFilter(aSampler.mag);
	c.mipmap = textureWrapToVkSamplerMipmapMode(aSampler.mipmap);
	c.wrapU = textureWrapToVkSamplerAddressMode(aSampler.wrapU);
	c.wrapV = textureWrapToVkSamplerAddressMode(aSampler.wrapV);
	c.wrapW = textureWrapToVkSamplerAddressMode(aSampler.wrapW);
	c.minLod = aSampler.minLod;
	c.maxLod = aSampler.maxLod;
	return c;
}
VkFormat converter::imageFormatToRvkFormat(const Image::Format aFormat)
{
	switch (aFormat) {
	case Image::Format::R8_UNORM: return VK_FORMAT_R8_UNORM;
	case Image::Format::RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
	case Image::Format::RGB8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
	case Image::Format::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
	case Image::Format::RGB8_SRGB: return VK_FORMAT_R8G8B8_SRGB;
	case Image::Format::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
	case Image::Format::RGB16_UNORM: return VK_FORMAT_R16G16B16_UNORM;
	case Image::Format::RGBA16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
	
	case Image::Format::R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
	case Image::Format::RG32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
	case Image::Format::RGB32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
	case Image::Format::RGBA32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;

	case Image::Format::R64_FLOAT: return VK_FORMAT_R64_SFLOAT;
	case Image::Format::RG64_FLOAT: return VK_FORMAT_R64G64_SFLOAT;
	case Image::Format::RGB64_FLOAT: return VK_FORMAT_R64G64B64_SFLOAT;
	case Image::Format::RGBA64_FLOAT: return VK_FORMAT_R64G64B64A64_SFLOAT;
	default: return VK_FORMAT_R8G8B8A8_UNORM;
	}
}

void converter::createImguiDescriptor(rvk::Descriptor *aDesc, rvk::Image* aImg)
{
	VkSampler s = aImg->getSampler();
	aImg->setSampler({ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT });
	
	aDesc->addImageSampler(0, rvk::Shader::Stage::FRAGMENT);
	aDesc->setImage(0, aImg);
	aDesc->finish(true);
	aImg->setSampler(s);
}
