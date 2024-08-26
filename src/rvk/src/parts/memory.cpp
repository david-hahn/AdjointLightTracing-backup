#include <rvk/parts/memory.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

Memory::Memory(LogicalDevice* aDevice, const VkDeviceSize aAllocationSize, const uint32_t aMemoryTypeIndex, const VkMemoryAllocateFlags aFlags, const uint32_t aDeviceMask) :
mDevice(aDevice), mMemory(nullptr), mAllocationSize(aAllocationSize), mMemoryTypeIndex(aMemoryTypeIndex), mFlags(aFlags), mDeviceMask(aDeviceMask)
{

	VkMemoryAllocateFlagsInfo afi{};
	afi.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	afi.flags = aFlags;
	afi.deviceMask = aDeviceMask;

	const VkMemoryAllocateInfo mai = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		(aFlags == 0) ? VK_NULL_HANDLE : &afi,
		aAllocationSize,
		aMemoryTypeIndex
	};

	VK_CHECK(mDevice->vk.AllocateMemory(mDevice->getHandle(), &mai, VK_NULL_HANDLE, &mMemory), "failed to allocate Image Memory!");
}

Memory::~Memory()
{
	if (mMemory != VK_NULL_HANDLE) {
		mDevice->vk.FreeMemory(mDevice->getHandle(), mMemory, VK_NULL_HANDLE);
		mMemory = VK_NULL_HANDLE;
	}
}

VkDeviceSize Memory::allocationSize() const
{
	return mAllocationSize;
}

uint32_t Memory::memoryTypeIndex() const
{
	return mMemoryTypeIndex;
}

VkDeviceMemory Memory::getHandle() const
{
	return mMemory;
}
