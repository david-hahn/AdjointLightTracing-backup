#pragma once
#include <tamashii/engine/scene/material.hpp>
#include <tamashii/engine/scene/image.hpp>
#include <cuda_runtime.h>

T_BEGIN_NAMESPACE
namespace converter {
	cudaTextureDesc samplerToCudaTexture(Image::Format aFormat, const Sampler& aSampler);
	cudaChannelFormatDesc imageFormatToCudaChannelFormatDesc(Image::Format aFormat);
}
T_END_NAMESPACE