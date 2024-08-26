#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class Fence
{
public:
						Fence(LogicalDevice* aDevice, VkFenceCreateFlags aFlags = 0);
						~Fence();
	VkResult			getStatus() const;
	void				reset() const;
	VkResult			wait(uint64_t aTimeout = UINT64_MAX) const;
	VkFence				getHandle() const;
	void				destroy() const;
private:
	LogicalDevice*		mDevice;
	VkFence				mFence;
};

RVK_END_NAMESPACE