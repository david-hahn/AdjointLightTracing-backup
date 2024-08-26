#include <tamashii/cuda_helper/convenience/cuda_type_converter.hpp>
T_USE_NAMESPACE
namespace {
	cudaTextureFilterMode textureWrapToVkSamplerFilter(const Sampler::Filter aFilter) {
		switch (aFilter) {
		case Sampler::Filter::NEAREST: return cudaFilterModePoint;
		case Sampler::Filter::LINEAR: return cudaFilterModeLinear;
		}
		return cudaFilterModeLinear;
	};
	cudaTextureFilterMode textureWrapToVkSamplerMipmapMode(const Sampler::Filter aFilter) {
		switch (aFilter) {
		case Sampler::Filter::NEAREST: return cudaFilterModePoint;
		case Sampler::Filter::LINEAR: return cudaFilterModeLinear;
		}
		return cudaFilterModeLinear;
	};
	cudaTextureAddressMode textureWrapToVkSamplerAddressMode(const Sampler::Wrap aWrap) {
		switch (aWrap) {
		case Sampler::Wrap::REPEAT: return cudaAddressModeWrap;
		case Sampler::Wrap::MIRRORED_REPEAT: return cudaAddressModeMirror;
		case Sampler::Wrap::CLAMP_TO_EDGE: return cudaAddressModeClamp;
		case Sampler::Wrap::CLAMP_TO_BORDER: return cudaAddressModeBorder;
		}
		return cudaAddressModeClamp;
	};
}
cudaTextureDesc tamashii::converter::samplerToCudaTexture(const Image::Format aFormat, const Sampler& aSampler)
{
	cudaTextureDesc c = {};
	c.addressMode[0] = textureWrapToVkSamplerAddressMode(aSampler.wrapU);
	c.addressMode[1] = textureWrapToVkSamplerAddressMode(aSampler.wrapV);
	c.addressMode[2] = textureWrapToVkSamplerAddressMode(aSampler.wrapW);
	c.filterMode = textureWrapToVkSamplerFilter(aSampler.min);
	c.readMode = cudaReadModeNormalizedFloat;
	c.normalizedCoords = 1;
	c.maxAnisotropy = 1;
	c.maxMipmapLevelClamp = aSampler.maxLod;
	c.minMipmapLevelClamp = aSampler.minLod;
	c.mipmapFilterMode = textureWrapToVkSamplerMipmapMode(aSampler.mipmap);
	c.borderColor[0] = 0.0f;
	c.borderColor[1] = 0.0f;
	c.borderColor[2] = 0.0f;
	c.borderColor[3] = 0.0f;
	switch (aFormat) {
		default: break;
		case Image::Format::RGB8_SRGB:
		case Image::Format::RGBA8_SRGB:
		c.sRGB = 1;
	}
	return c;
}

cudaChannelFormatDesc tamashii::converter::imageFormatToCudaChannelFormatDesc(const Image::Format aFormat)
{
	switch (aFormat) {
	case Image::Format::R8_UNORM: return cudaCreateChannelDesc<uchar1>();
	case Image::Format::RG8_UNORM: return cudaCreateChannelDesc<uchar2>();
	case Image::Format::RGB8_UNORM: return cudaCreateChannelDesc<uchar3>();
	case Image::Format::RGBA8_UNORM: return cudaCreateChannelDesc<uchar4>();
	case Image::Format::RGB8_SRGB: return cudaCreateChannelDesc<uchar3>();
	case Image::Format::RGBA8_SRGB: return cudaCreateChannelDesc<uchar4>();
	case Image::Format::RGB16_UNORM: return cudaCreateChannelDesc<ushort3>();
	case Image::Format::RGBA16_UNORM: return cudaCreateChannelDesc<ushort4>();
	
	case Image::Format::R32_FLOAT: return cudaCreateChannelDesc<float1>();
	case Image::Format::RG32_FLOAT: return cudaCreateChannelDesc<float2>();
	case Image::Format::RGB32_FLOAT: return cudaCreateChannelDesc<float3>();
	case Image::Format::RGBA32_FLOAT: return cudaCreateChannelDesc<float4>();

	case Image::Format::R64_FLOAT: return cudaCreateChannelDesc<double1>();
	case Image::Format::RG64_FLOAT: return cudaCreateChannelDesc<double2>();
	case Image::Format::RGB64_FLOAT: return cudaCreateChannelDesc<double3>();
	case Image::Format::RGBA64_FLOAT: return cudaCreateChannelDesc<double4>();
	default: return cudaCreateChannelDesc<uchar4>();
	}
}
