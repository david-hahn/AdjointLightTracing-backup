#include <rvk/parts/command_pool.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

CommandPool::CommandPool(LogicalDevice* aDevice, const uint32_t aQueueFamilyIndex, const VkCommandPoolCreateFlags aFlags) : mDevice(aDevice), mCommandPool(VK_NULL_HANDLE), mQueueFamilyIndex(aQueueFamilyIndex), mFlags(aFlags)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = mQueueFamilyIndex;
	poolInfo.flags = mFlags;
	VK_CHECK(mDevice->vk.CreateCommandPool(mDevice->getHandle(), &poolInfo, NULL, &mCommandPool), "failed to create command pool!");
}

CommandPool::~CommandPool()
{
	destroy();
}

VkCommandPool CommandPool::getHandle() const
{
	return mCommandPool;
}

uint32_t CommandPool::getQueueFamilyIndex() const
{
	return mQueueFamilyIndex;
}

std::vector<CommandBuffer*> CommandPool::allocCommandBuffers(const uint32_t aCount, const VkCommandBufferLevel aLevel)
{
	std::vector<VkCommandBuffer> vkCommandBuffers(aCount);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = aLevel;
	allocInfo.commandPool = mCommandPool;
	allocInfo.commandBufferCount = aCount;
	VK_CHECK(mDevice->vk.AllocateCommandBuffers(mDevice->getHandle(), &allocInfo, vkCommandBuffers.data()), "failed to allocate CMD Buffer!");

	std::vector<CommandBuffer*> commandBuffers;
	commandBuffers.reserve(aCount);
	for(const VkCommandBuffer &vcb : vkCommandBuffers)
	{
		mCommandBuffers.push_back(new CommandBuffer(mDevice, this, aLevel, vcb));
		commandBuffers.push_back(mCommandBuffers.back());
	}
	return commandBuffers;
}

void CommandPool::freeCommandBuffers(const std::vector<CommandBuffer*>& aCommandBuffers)
{
	std::vector<CommandBuffer*> commandBuffers = aCommandBuffers;
	freeCommandBuffers(commandBuffers);
}

void CommandPool::freeCommandBuffers(std::vector<CommandBuffer*>& aCommandBuffers)
{
	std::vector<VkCommandBuffer> freeCbs;
	freeCbs.reserve(aCommandBuffers.size());
	for (auto& commandBuffer : aCommandBuffers)
	{
		
		auto it = mCommandBuffers.begin();
		while (it != mCommandBuffers.end())
		{
			if ((*it) == commandBuffer) {
				const CommandBuffer* cb = (*it);
				freeCbs.push_back(cb->mCommandBuffer);
				mCommandBuffers.erase(it);
				delete cb;
				commandBuffer = nullptr;
				break;
			}
			std::advance(it, 1);
		}
	}
	if (!freeCbs.empty()) mDevice->vk.FreeCommandBuffers(mDevice->getHandle(), mCommandPool, static_cast<uint32_t>(freeCbs.size()), freeCbs.data());
}

void CommandPool::reset(const VkCommandPoolResetFlags aFlags) const
{
	mDevice->vk.ResetCommandPool(mDevice->getHandle(), mCommandPool, aFlags);
}

void CommandPool::destroy()
{
	if (mDevice && mCommandPool) mDevice->vk.DestroyCommandPool(mDevice->getHandle(), mCommandPool, nullptr);
	mCommandPool = nullptr;
	mDevice = nullptr;
}
