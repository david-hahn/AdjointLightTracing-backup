#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/sampler_config.hpp>

#include <deque>
#include <unordered_map>

RVK_BEGIN_NAMESPACE
class PhysicalDevice;
class Swapchain;
class CommandPool;
class Fence;
class Semaphore;
class Queue;
class QueueFamily;
class Sampler;




typedef std::pair<CommandPool*, Queue*> StlBundle;

class LogicalDevice {
public:
	bool											isExtensionActive(const char* aExtension) const;

	int												getQueueFamilyCount() const;
	bool											checkQueueFamily(int aFamilyIndex, uint32_t aQueueFlags, const std::vector<void*>& aPresentationSupport = {}) const;
	int												getQueueFamilyIndex(uint32_t aQueueFlags, const std::vector<void*>& aPresentationSupport = {}) const;
	int												getQueueCount(uint32_t aQueueFamilyIndex) const;
	QueueFamily*									getQueueFamily(uint32_t aQueueFamilyIndex);
	Queue*											getQueue(uint32_t aQueueFamilyIndex, uint32_t aQueueIndex);

	CommandPool*									createCommandPool(uint32_t aQueueFamilyIndex, VkCommandPoolCreateFlags aFlags = 0);
	void											destroyCommandPool(CommandPool*& aCommandPool);

	Semaphore*										createSemaphore(VkSemaphoreType aType = VK_SEMAPHORE_TYPE_BINARY, uint64_t aInitialValue = 0);
	std::deque<Semaphore*>&							getSemaphores();
	void											destroySemaphore(Semaphore*& aSemaphore);

	Fence*											createFence(VkFenceCreateFlags aFlags = 0);
	std::deque<Fence*>&								getFences();
	void											destroyFence(Fence*& aFence);

	VkPhysicalDeviceMemoryBudgetPropertiesEXT		getMemoryBudgetProperties() const;

	void											waitIdle() const;

	VkMemoryRequirements							getBufferMemoryRequirements(VkBuffer aBuffer) const;
	VkMemoryRequirements							getImageMemoryRequirements(VkImage aImage) const;
	int												findMemoryTypeIndex(VkMemoryPropertyFlags aMemoryProperties, uint32_t aMemoryTypeBits) const;
	[[nodiscard]] VkDeviceMemory					allocMemory(VkDeviceSize aAllocationSize, uint32_t aMemoryTypeIndex) const;

	Sampler*										getSampler(SamplerConfig aConfig);

	VkDevice										getHandle() const;
	PhysicalDevice*									getPhysicalDevice() const;

	struct vk_functions {
#define VK_DEVICE_LEVEL_FUNCTION(fn) PFN_vk##fn fn = nullptr;
#include <rvk/vk_function_list.inl>
#undef VK_DEVICE_LEVEL_FUNCTION
	} vk;
	struct vk_functions_cmd {
#define VK_DEVICE_LEVEL_CMD_FUNCTION(fn) PFN_vkCmd##fn fn = nullptr;
#include <rvk/vk_function_list.inl>
#undef VK_DEVICE_LEVEL_CMD_FUNCTION
	} vkCmd;

private:
	friend class Instance;
	friend class PhysicalDevice;

													LogicalDevice(PhysicalDevice* aPhysicalDevice, VkDevice aDevice);
													~LogicalDevice();

	PhysicalDevice*									mPhysicalDevice;

	std::vector<const char*>						mActiveDeviceExtensions;
	std::vector<QueueFamily>						mQueueFamilies;
	std::deque<Semaphore*>							mSemaphores;
	std::deque<Fence*>								mFences;
	
	std::unordered_map<SamplerConfig, Sampler*, SamplerConfig> mSamplers;

	VkDevice										mLogicalDevice;
};
RVK_END_NAMESPACE
