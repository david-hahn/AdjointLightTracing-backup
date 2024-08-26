#include <rvk/parts/queue.hpp>
#include <rvk/parts/queue_family.hpp>
#include <rvk/parts/command_buffer.hpp>
#include <rvk/parts/semaphore.hpp>
#include <rvk/parts/fence.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

Queue::Queue(LogicalDevice* aDevice, QueueFamily* aQueueFamily, const uint32_t aIndex, const float aPriority) : mDevice(aDevice), mQueueFamily(aQueueFamily), mQueue(VK_NULL_HANDLE), mIndex(aIndex), mPriority(aPriority)
{
	mDevice->vk.GetDeviceQueue(aDevice->getHandle(), aQueueFamily->getIndex(), aIndex, &mQueue);
}

uint32_t Queue::getIndex() const
{
	return mIndex;
}

float Queue::getPriority() const
{
	return mPriority;
}

QueueFamily* Queue::getFamily() const
{
	return mQueueFamily;
}

void Queue::submitCommandBuffers(const std::vector<CommandBuffer*>& aCommandBuffers, Fence* aFence,
                                 const std::vector<std::pair<Semaphore*, VkPipelineStageFlags>>& aWaitSemaphore,
                                 const std::vector<Semaphore*>& aSignalSemaphore) const
{
	std::vector<VkCommandBuffer> cbList(aCommandBuffers.size());
	std::vector<VkSemaphore> sWait(aWaitSemaphore.size());
	std::vector<VkPipelineStageFlags> sStage(aWaitSemaphore.size());
	std::vector<VkSemaphore> sSignal(aSignalSemaphore.size());

	for (uint32_t i = 0; i < aCommandBuffers.size(); i++) cbList[i] = aCommandBuffers[i]->getHandle();
	for (uint32_t i = 0; i < aWaitSemaphore.size(); i++) sWait[i] = aWaitSemaphore[i].first->getHandle();
	for (uint32_t i = 0; i < aWaitSemaphore.size(); i++) sStage[i] = aWaitSemaphore[i].second;
	for (uint32_t i = 0; i < aSignalSemaphore.size(); i++) sSignal[i] = aSignalSemaphore[i]->getHandle();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(sWait.size());
	submitInfo.pWaitSemaphores = sWait.data();
	submitInfo.pWaitDstStageMask = sStage.data();
	submitInfo.commandBufferCount = static_cast<uint32_t>(cbList.size());
	submitInfo.pCommandBuffers = cbList.data();
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(sSignal.size());
	submitInfo.pSignalSemaphores = sSignal.data();
	VK_CHECK(mDevice->vk.QueueSubmit(mQueue, 1, &submitInfo, aFence ? aFence->getHandle() : VK_NULL_HANDLE), "failed to submit Queue!");
}

#ifdef VK_KHR_surface
VkResult Queue::present(const std::vector<Swapchain*>& aSwapchains, const std::vector<Semaphore*>& aWaitSemaphores) const
{
	std::vector<VkSwapchainKHR> scList(aSwapchains.size());
	std::vector<uint32_t> scIdxList(aSwapchains.size());
	std::vector<VkSemaphore> sWait(aWaitSemaphores.size());
	std::vector<VkResult> results(aSwapchains.size());

	for (uint32_t i = 0; i < scList.size(); i++) scList[i] = aSwapchains[i]->getHandle();
	for (uint32_t i = 0; i < scIdxList.size(); i++) scIdxList[i] = aSwapchains[i]->getCurrentIndex();
	for (uint32_t i = 0; i < sWait.size(); i++) sWait[i] = aWaitSemaphores[i]->getHandle();

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = static_cast<uint32_t>(aWaitSemaphores.size());
	presentInfo.pWaitSemaphores = sWait.data();
	presentInfo.swapchainCount = static_cast<uint32_t>(aSwapchains.size());
	presentInfo.pSwapchains = scList.data();
	presentInfo.pImageIndices = scIdxList.data();
	presentInfo.pResults = results.data();

	return mDevice->vk.QueuePresentKHR(mQueue, &presentInfo);
}
#endif /*VK_KHR_surface*/

void Queue::waitIdle() const
{
	VK_CHECK(mDevice->vk.QueueWaitIdle(mQueue), "failed to wait for Queue execution!");
}

VkQueue Queue::getHandle() const
{
	return mQueue;
}
