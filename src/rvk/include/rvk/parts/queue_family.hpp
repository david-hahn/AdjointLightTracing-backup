#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class Queue;
class CommandPool;
class QueueFamily
{
public:
								QueueFamily(LogicalDevice* aDevice, uint32_t aIndex, std::vector<float>& aPriorities);
								~QueueFamily();

	uint32_t					getIndex() const;
	Queue*						getQueue(uint32_t aIndex) const;
	uint32_t					getQueueCount() const;
	CommandPool*				createCommandPool(VkCommandPoolCreateFlags aFlags);
	void						destroyCommandPool(CommandPool*& aCommandPool);
	std::vector<Queue*>&		getQueues();
	std::deque<CommandPool*>&	getCommandPools();
private:
	LogicalDevice*				mDevice;
	uint32_t					mIndex;
	std::vector<Queue*>			mQueues;
	std::deque<CommandPool*>	mCommandPools;
};

RVK_END_NAMESPACE