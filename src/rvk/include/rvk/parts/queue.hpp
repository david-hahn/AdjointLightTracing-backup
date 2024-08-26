#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class QueueFamily;
class CommandBuffer;
class Semaphore;
class Fence;
class Swapchain;
class Queue
{
public:
								Queue(LogicalDevice* aDevice, QueueFamily* aQueueFamily, uint32_t aIndex, float aPriority);
								~Queue() = default;

	uint32_t					getIndex() const;
	float						getPriority() const;
	QueueFamily*				getFamily() const;
	void						submitCommandBuffers(const std::vector<CommandBuffer*>& aCommandBuffers, Fence* aFence = nullptr,
									const std::vector<std::pair<Semaphore*, VkPipelineStageFlags>>& aWaitSemaphore = {},
									const std::vector<Semaphore*>& aSignalSemaphore = {}) const;
#ifdef VK_KHR_surface
	VkResult					present(const std::vector<Swapchain*>& aSwapchains, const std::vector<Semaphore*>& aWaitSemaphores) const;
#endif /*VK_KHR_surface*/
	void						waitIdle() const;
	VkQueue						getHandle() const;
private:
	LogicalDevice*				mDevice;
	QueueFamily*				mQueueFamily;
	VkQueue						mQueue;

	uint32_t					mIndex;
	float						mPriority;
};

RVK_END_NAMESPACE