#pragma once
#include <tamashii/core/scene/image.hpp>
#include <rvk/rvk.hpp>

T_BEGIN_NAMESPACE
namespace converter {
	rvk::SamplerConfig samplerToRvkSampler(const Sampler& aSampler);
	VkFormat imageFormatToRvkFormat(Image::Format aFormat);

	
	void createImguiDescriptor(rvk::Descriptor* aDesc, rvk::Image* aImg);
}
T_END_NAMESPACE
