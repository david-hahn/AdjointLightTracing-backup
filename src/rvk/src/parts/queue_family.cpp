#include <rvk/parts/queue_family.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE
QueueFamily::QueueFamily(LogicalDevice* aDevice, const uint32_t aIndex, std::vector<float>& aPriorities) : mDevice(aDevice), mIndex(aIndex)
{
	mQueues.reserve(aPriorities.size());
	for (uint32_t i = 0; i < aPriorities.size(); i++)
	{
		mQueues.push_back(new Queue(aDevice, this, i, aPriorities[i]));
	}
}

QueueFamily::~QueueFamily()
{
	for (const CommandPool* pool : mCommandPools) delete pool;
	for (const Queue* queue : mQueues) delete queue;
	mCommandPools.clear();
	mQueues.clear();
}

uint32_t QueueFamily::getIndex() const
{
	return mIndex;
}

Queue* QueueFamily::getQueue(const uint32_t aIndex) const
{
	return mQueues[aIndex];
}

uint32_t QueueFamily::getQueueCount() const
{
	return static_cast<uint32_t>(mQueues.size());
}

CommandPool* QueueFamily::createCommandPool(const VkCommandPoolCreateFlags aFlags)
{
	mCommandPools.push_back(new CommandPool(mDevice, mIndex, aFlags));
	return mCommandPools.back();
}

void QueueFamily::destroyCommandPool(CommandPool*& aCommandPool)
{
	const uint32_t familyIndex = aCommandPool->getQueueFamilyIndex();
	auto it = mCommandPools.begin();
	while (it != mCommandPools.end())
	{
		if ((*it) == aCommandPool) {
			mCommandPools.erase(it);
			delete aCommandPool;
			aCommandPool = nullptr;
			return;
		}
		std::advance(it, 1);
	}
}

std::vector<Queue*>& QueueFamily::getQueues()
{
	return mQueues;
}

std::deque<CommandPool*>& QueueFamily::getCommandPools()
{
	return mCommandPools;
}