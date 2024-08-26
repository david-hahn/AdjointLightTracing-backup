#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class Memory
{
public:
							Memory(LogicalDevice* aDevice, VkDeviceSize aAllocationSize, uint32_t aMemoryTypeIndex, VkMemoryAllocateFlags aFlags = 0, uint32_t aDeviceMask = 0);
							~Memory();

	VkDeviceSize			allocationSize() const;
	uint32_t				memoryTypeIndex() const;
	VkDeviceMemory			getHandle() const;
private:
	LogicalDevice*			mDevice;
	VkDeviceMemory			mMemory;
	VkDeviceSize			mAllocationSize;
	uint32_t				mMemoryTypeIndex;
	VkMemoryAllocateFlags	mFlags;
	uint32_t				mDeviceMask;
};
RVK_END_NAMESPACE