#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE

class LogicalDevice;
class CommandBuffer;
class CommandPool {
public:
												CommandPool(LogicalDevice* aDevice, uint32_t aQueueFamilyIndex, VkCommandPoolCreateFlags aFlags);
												~CommandPool();
	VkCommandPool								getHandle() const;
	uint32_t									getQueueFamilyIndex() const;
	std::vector<CommandBuffer*>					allocCommandBuffers(uint32_t aCount, VkCommandBufferLevel aLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	void										freeCommandBuffers(const std::vector<CommandBuffer*>& aCommandBuffers);
	void										freeCommandBuffers(std::vector<CommandBuffer*>& aCommandBuffers);
	void										reset(VkCommandPoolResetFlags aFlags = 0) const;

	void										destroy();
private:
	LogicalDevice*								mDevice;
	VkCommandPool								mCommandPool;
	uint32_t									mQueueFamilyIndex;
	VkCommandPoolCreateFlags					mFlags;
	std::deque<CommandBuffer*>					mCommandBuffers;
};

RVK_END_NAMESPACE