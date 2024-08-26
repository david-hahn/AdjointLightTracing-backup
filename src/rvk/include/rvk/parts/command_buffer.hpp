#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class CommandPool;
class Queue;
class Buffer;
class Image;

#define RVK_L2 VK_ATTACHMENT_LOAD_OP_LOAD
#define RVK_LC VK_ATTACHMENT_LOAD_OP_CLEAR
#define RVK_LDC VK_ATTACHMENT_LOAD_OP_DONT_CARE
#define RVK_S2 VK_ATTACHMENT_STORE_OP_STORE
#define RVK_SDC VK_ATTACHMENT_STORE_OP_DONT_CARE

struct RenderInfo {
	rvk::Image* image;
	VkClearColorValue			clearValue;
	VkAttachmentLoadOp			loadOp;
	VkAttachmentStoreOp			storeOp;
};

struct RenderInfoDepth {
	rvk::Image* image;
	VkClearDepthStencilValue	clearValue;
	VkAttachmentLoadOp			loadOp;
	VkAttachmentStoreOp			storeOp;
};

class CommandBuffer
{
public:
												CommandBuffer(LogicalDevice* aDevice, CommandPool* aCommandPool, VkCommandBufferLevel aLevel);
												CommandBuffer(LogicalDevice* aDevice, CommandPool* aCommandPool, VkCommandBufferLevel aLevel, VkCommandBuffer aCommandBuffer);
												~CommandBuffer() = default;

	void										begin(VkCommandBufferUsageFlags aFlags);
	void										end();
	void										reset(VkCommandBufferResetFlags aFlags = 0) const;
	VkCommandBuffer								getHandle() const;
	LogicalDevice*								getDevice() const;
	uint32_t									getQueueFamilyIndex() const;
	CommandPool*								getPool() const;


	void										cmdMemoryBarrier(VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask, VkAccessFlags aSrcAccessMask = 0, VkAccessFlags aDstAccessMask = 0);
	void										cmdBufferMemoryBarrier(const Buffer* aBuffer, VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask, VkAccessFlags aSrcAccessMask = 0, VkAccessFlags aDstAccessMask = 0);

	void										cmdBeginRendering(const std::vector<RenderInfo>& aColorAttachments, const RenderInfoDepth&& aDepthAttachments = {}) const;
	void										cmdEndRendering() const;

private:
	friend class CommandPool;
	bool										mRecording;
	VkCommandBufferUsageFlags					mFlags;
	LogicalDevice*								mDevice;
	VkCommandBufferLevel						mLevel;
	CommandPool*								mCommandPool;
	VkCommandBuffer								mCommandBuffer;
};

class SingleTimeCommand
{
public:
							SingleTimeCommand(CommandPool* aCommandPool, Queue* aQueue);
							~SingleTimeCommand();
	void					begin();
	CommandBuffer*			buffer() const;
	void					end();
							
	void					execute(const std::function<void(CommandBuffer*)>& aFunction);

private:
	CommandPool*			mCommandPool;
	Queue*					mQueue;

	CommandBuffer*			mCommandBuffer;
};
RVK_END_NAMESPACE