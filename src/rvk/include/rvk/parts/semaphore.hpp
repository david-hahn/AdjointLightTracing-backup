#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class Semaphore
{
public:
						Semaphore(LogicalDevice* aDevice, VkSemaphoreType aType = VK_SEMAPHORE_TYPE_BINARY, uint64_t aInitialValue = 0);
						~Semaphore();
	VkSemaphore			getHandle() const;
	void				signal(uint64_t aValue) const;
	uint64_t			getCounterValue() const;
	void				destroy();
private:
	LogicalDevice*		mDevice;
	VkSemaphoreType		mType;
	VkSemaphore			mSemaphore;
};

RVK_END_NAMESPACE