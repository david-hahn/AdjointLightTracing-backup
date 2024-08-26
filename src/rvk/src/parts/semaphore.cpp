#include <rvk/parts/semaphore.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

Semaphore::Semaphore(LogicalDevice* aDevice, VkSemaphoreType aType, uint64_t aInitialValue) : mDevice(aDevice), mType(aType), mSemaphore(VK_NULL_HANDLE)
{
	VkSemaphoreTypeCreateInfo stci = {};
	stci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	stci.semaphoreType = aType;
	stci.initialValue = aInitialValue;

	VkSemaphoreCreateInfo sci = {};
	sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	sci.pNext = aType != VK_SEMAPHORE_TYPE_BINARY ? &stci : VK_NULL_HANDLE;
	VK_CHECK(mDevice->vk.CreateSemaphore(mDevice->getHandle(), &sci, NULL, &mSemaphore), "failed to create Semaphore!");
}

Semaphore::~Semaphore()
{
	destroy();
}


VkSemaphore Semaphore::getHandle() const
{
	return mSemaphore;
}

void Semaphore::signal(uint64_t aValue) const
{
	if (mType != VK_SEMAPHORE_TYPE_TIMELINE) return;
	VkSemaphoreSignalInfo si = {};
	si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	si.semaphore = mSemaphore;
	si.value = aValue;
	mDevice->vk.SignalSemaphore(mDevice->getHandle(), &si);
}

uint64_t Semaphore::getCounterValue() const
{
	if (mType != VK_SEMAPHORE_TYPE_TIMELINE) return 0;
	uint64_t value;
	mDevice->vk.GetSemaphoreCounterValue(mDevice->getHandle(), mSemaphore, &value);
	return value;
}

void Semaphore::destroy()
{
	if (mSemaphore) {
		mDevice->vk.DestroySemaphore(mDevice->getHandle(), mSemaphore, nullptr);
		mSemaphore = VK_NULL_HANDLE;
	}
}
