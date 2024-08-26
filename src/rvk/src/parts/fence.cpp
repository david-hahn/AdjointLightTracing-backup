#include <rvk/parts/fence.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

Fence::Fence(LogicalDevice* aDevice, VkFenceCreateFlags aFlags): mDevice(aDevice), mFence(VK_NULL_HANDLE)
{
	VkFenceCreateInfo fci = {};
	fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fci.flags = aFlags;
	VK_CHECK(mDevice->vk.CreateFence(mDevice->getHandle(), &fci, nullptr, &mFence), "failed to create Fence!");
}

Fence::~Fence()
{
	destroy();
}

VkResult Fence::getStatus() const
{
	return mDevice->vk.GetFenceStatus(mDevice->getHandle(), mFence);
}

void Fence::reset() const
{
	VK_CHECK(mDevice->vk.ResetFences(mDevice->getHandle(), 1, &mFence), "failed to reset Fence!");
}

VkResult Fence::wait(const uint64_t aTimeout) const
{
	return mDevice->vk.WaitForFences(mDevice->getHandle(), 1, &mFence, VK_TRUE, aTimeout);
}

VkFence Fence::getHandle() const
{
	return mFence;
}

void Fence::destroy() const
{
	mDevice->vk.DestroyFence(mDevice->getHandle(), mFence, nullptr);
}
