#include <rvk/parts/device_logical.hpp>
#include <rvk/parts/semaphore.hpp>
#include <rvk/parts/fence.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

LogicalDevice::LogicalDevice(PhysicalDevice* aPhysicalDevice, const VkDevice aDevice) : mPhysicalDevice{ aPhysicalDevice }, mSamplers{ 40 }, mLogicalDevice{ aDevice }
{
	const Instance* instance = mPhysicalDevice->getInstance();
#define VK_DEVICE_LEVEL_FUNCTION(fn) vk.fn = (PFN_vk##fn) instance->vk.GetDeviceProcAddr( mLogicalDevice , "vk"#fn );
#define VK_DEVICE_LEVEL_CMD_FUNCTION(fn) vkCmd.fn = (PFN_vkCmd##fn) instance->vk.GetDeviceProcAddr( mLogicalDevice , "vkCmd"#fn );
#include <rvk/vk_function_list.inl>
#undef VK_DEVICE_LEVEL_FUNCTION
#undef VK_DEVICE_LEVEL_CMD_FUNCTION
}

LogicalDevice::~LogicalDevice()
{
	mQueueFamilies.clear();
	for (const Semaphore* s : mSemaphores) delete s;
	mSemaphores.clear();
	for (const Fence* f : mFences) delete f;
	mFences.clear();
	for (const std::pair<SamplerConfig, Sampler*> p : mSamplers) delete p.second;
	mSamplers.clear();

    if(mLogicalDevice) vk.DestroyDevice(mLogicalDevice, nullptr);
}

bool LogicalDevice::isExtensionActive(const char * aExtension) const
{
	for (const auto activeDeviceExtension : mActiveDeviceExtensions)
	{
		if (!strcmp(aExtension, activeDeviceExtension)) return true;
	}
	return false;
}

int LogicalDevice::getQueueFamilyCount() const
{
	return static_cast<int>(mQueueFamilies.size());
}

bool LogicalDevice::checkQueueFamily(const int aFamilyIndex, const uint32_t aQueueFlags,
	const std::vector<void*>& aPresentationSupport) const
{
	return mPhysicalDevice->checkQueueFamily(aFamilyIndex, aQueueFlags, aPresentationSupport);
}

int LogicalDevice::getQueueFamilyIndex(const uint32_t aQueueFlags, const std::vector<void*>& aPresentationSupport) const
{
	return mPhysicalDevice->getQueueFamilyIndex(aQueueFlags, aPresentationSupport);
}

int LogicalDevice::getQueueCount(const uint32_t aQueueFamilyIndex) const
{
	if(aQueueFamilyIndex == -1) return 0;
	return static_cast<int>(mQueueFamilies[aQueueFamilyIndex].getQueueCount());
}

QueueFamily* LogicalDevice::getQueueFamily(uint32_t aQueueFamilyIndex)
{
	return &mQueueFamilies[aQueueFamilyIndex];
}

Queue* LogicalDevice::getQueue(const uint32_t aQueueFamilyIndex, const uint32_t aQueueIndex)
{
	return mQueueFamilies[aQueueFamilyIndex].getQueue(aQueueIndex);
}

CommandPool* LogicalDevice::createCommandPool(uint32_t aQueueFamilyIndex, VkCommandPoolCreateFlags aFlags)
{
	return mQueueFamilies[aQueueFamilyIndex].createCommandPool(aFlags);
}

void LogicalDevice::destroyCommandPool(CommandPool*& aCommandPool)
{
	const uint32_t family_index = aCommandPool->getQueueFamilyIndex();
	mQueueFamilies[family_index].destroyCommandPool(aCommandPool);
}

Semaphore* LogicalDevice::createSemaphore(VkSemaphoreType aType, uint64_t aInitialValue)
{
	mSemaphores.push_back(new Semaphore(this, aType, aInitialValue));
	return mSemaphores.back();
}

std::deque<Semaphore*>& LogicalDevice::getSemaphores()
{
	return mSemaphores;
}

void LogicalDevice::destroySemaphore(Semaphore*& aSemaphore)
{
	const auto it = std::find(mSemaphores.begin(), mSemaphores.end(), aSemaphore);
	if (it != mSemaphores.end()) {
		delete aSemaphore;
		mSemaphores.erase(it);
		aSemaphore = nullptr;
	}
}

Fence* LogicalDevice::createFence(VkFenceCreateFlags aFlags)
{
	mFences.push_back(new Fence(this, aFlags));
	return mFences.back();
}

std::deque<Fence*>& LogicalDevice::getFences()
{
	return mFences;
}

void LogicalDevice::destroyFence(Fence*& aFence)
{
	const auto it = std::find(mFences.begin(), mFences.end(), aFence);
	if (it != mFences.end()) {
		delete aFence;
		mFences.erase(it);
		aFence = nullptr;
	}
}

VkPhysicalDeviceMemoryBudgetPropertiesEXT LogicalDevice::getMemoryBudgetProperties() const
{
	VkPhysicalDeviceMemoryBudgetPropertiesEXT s = {};
	s.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
	VkPhysicalDeviceMemoryProperties2 p2 = {};
	p2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	p2.pNext = &s;
	getPhysicalDevice()->getInstance()->vk.GetPhysicalDeviceMemoryProperties2(getPhysicalDevice()->getHandle(), &p2);
	return s;
}

void LogicalDevice::waitIdle() const
{
	VK_CHECK(vk.DeviceWaitIdle(mLogicalDevice), "failed to wait for Device idle!");
}

VkMemoryRequirements LogicalDevice::getBufferMemoryRequirements(const VkBuffer aBuffer) const
{
	VkMemoryRequirements memReq = {};
	vk.GetBufferMemoryRequirements(mLogicalDevice, aBuffer, &memReq);
	return memReq;
}

VkMemoryRequirements LogicalDevice::getImageMemoryRequirements(const VkImage aImage) const
{
	VkMemoryRequirements memReq = {};
	vk.GetImageMemoryRequirements(mLogicalDevice, aImage, &memReq);
	return memReq;
}

int LogicalDevice::findMemoryTypeIndex(const VkMemoryPropertyFlags aMemoryProperties, const uint32_t aMemoryTypeBits) const
{
	return mPhysicalDevice->findMemoryTypeIndex(aMemoryProperties, aMemoryTypeBits);
}

VkDeviceMemory LogicalDevice::allocMemory(const VkDeviceSize aAllocationSize, const uint32_t aMemoryTypeIndex) const
{
	/*VkExportMemoryAllocateInfo emai = {};
	emai.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
	emai.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;*/

	VkMemoryAllocateInfo mai = {};
	mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mai.allocationSize = aAllocationSize;
	mai.memoryTypeIndex = aMemoryTypeIndex;
	VkDeviceMemory memory;
	VK_CHECK(vk.AllocateMemory(mLogicalDevice, &mai, VK_NULL_HANDLE, &memory), "failed to allocate Image Memory!");
	return memory;
}

Sampler* LogicalDevice::getSampler(const SamplerConfig aConfig)
{
	const auto search = mSamplers.find(aConfig);
	if (search != mSamplers.end()) return search->second;

	auto sampler = new rvk::Sampler(this, aConfig);
	mSamplers.insert(std::pair(aConfig, sampler));
	return sampler;
}

VkDevice LogicalDevice::getHandle() const
{
	return mLogicalDevice;
}

PhysicalDevice* LogicalDevice::getPhysicalDevice() const
{
	return mPhysicalDevice;
}
