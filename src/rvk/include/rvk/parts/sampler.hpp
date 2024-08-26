#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/sampler_config.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class Sampler {
public:

					Sampler(LogicalDevice* aDevice, SamplerConfig aConfig);
					~Sampler();
	VkSampler		getHandle() const;

private:
	LogicalDevice*	mDevice;
	VkSampler		mSampler;
};
RVK_END_NAMESPACE