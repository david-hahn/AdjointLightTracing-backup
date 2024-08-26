#include <rvk/parts/sampler.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

Sampler::Sampler(LogicalDevice* aDevice, const SamplerConfig aConfig) : mDevice(aDevice), mSampler(VK_NULL_HANDLE)
{
	VkSamplerCreateInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	desc.pNext = nullptr;
	desc.flags = 0;
	desc.magFilter = aConfig.mag;
	desc.minFilter = aConfig.min;
	desc.mipmapMode = aConfig.mipmap;
	desc.addressModeU = aConfig.wrapU;
	desc.addressModeV = aConfig.wrapV;
	desc.addressModeW = aConfig.wrapW;
	desc.mipLodBias = 0.0f;
	desc.anisotropyEnable = VK_FALSE;
	desc.maxAnisotropy = 1;
	desc.compareEnable = VK_FALSE;
	desc.compareOp = VK_COMPARE_OP_ALWAYS;
	desc.minLod = aConfig.minLod;
	desc.maxLod = aConfig.maxLod;
	desc.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	desc.unnormalizedCoordinates = VK_FALSE;
	VK_CHECK(mDevice->vk.CreateSampler(mDevice->getHandle(), &desc, VK_NULL_HANDLE, &mSampler), "failed to create Sampler!");
}

Sampler::~Sampler()
{
	mDevice->vk.DestroySampler(mDevice->getHandle(), mSampler, VK_NULL_HANDLE);
}

VkSampler Sampler::getHandle() const
{
	return mSampler;
}