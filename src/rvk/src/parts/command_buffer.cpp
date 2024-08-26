#include <rvk/parts/command_buffer.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

CommandBuffer::CommandBuffer(LogicalDevice* aDevice, CommandPool* aCommandPool, const VkCommandBufferLevel aLevel) :
mRecording(false), mFlags(0), mDevice(aDevice), mLevel(aLevel), mCommandPool(aCommandPool), mCommandBuffer(VK_NULL_HANDLE)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = aLevel;
	allocInfo.commandPool = mCommandPool->getHandle();
	allocInfo.commandBufferCount = 1;
	VK_CHECK(mDevice->vk.AllocateCommandBuffers(mDevice->getHandle(), &allocInfo, &mCommandBuffer), "failed to allocate CMD Buffer!");
}

CommandBuffer::CommandBuffer(LogicalDevice* aDevice, CommandPool* aCommandPool, const VkCommandBufferLevel aLevel, const VkCommandBuffer aCommandBuffer) :
mRecording(false), mFlags(0), mDevice(aDevice), mLevel(aLevel), mCommandPool(aCommandPool), mCommandBuffer(aCommandBuffer)
{}

void CommandBuffer::begin(const VkCommandBufferUsageFlags aFlags)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = aFlags;
	VK_CHECK(mDevice->vk.BeginCommandBuffer(mCommandBuffer, &beginInfo), "failed to begin CMD Buffer!");
		mFlags = aFlags;
	mRecording = true;
}

void CommandBuffer::end()
{
	VK_CHECK(mDevice->vk.EndCommandBuffer(mCommandBuffer), "failed to end CMD Buffer!");
		mFlags = 0;
	mRecording = false;
}

void CommandBuffer::reset(const VkCommandBufferResetFlags aFlags) const
{
	mDevice->vk.ResetCommandBuffer(mCommandBuffer, aFlags);
}

VkCommandBuffer CommandBuffer::getHandle() const
{
	return mCommandBuffer;
}

LogicalDevice* CommandBuffer::getDevice() const
{
	return mDevice;
}

uint32_t CommandBuffer::getQueueFamilyIndex() const
{
	return mCommandPool->getQueueFamilyIndex();
}

CommandPool* CommandBuffer::getPool() const
{
	return mCommandPool;
}

void CommandBuffer::cmdMemoryBarrier(VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask,
	VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask)
{
	VkMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = aSrcAccessMask;
	barrier.dstAccessMask = aDstAccessMask;
	mDevice->vkCmd.PipelineBarrier(mCommandBuffer,
		aSrcStageMask,
		aDstStageMask,
		0,
		1, &barrier,
		0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);
}

void CommandBuffer::cmdBufferMemoryBarrier(const Buffer* aBuffer, VkPipelineStageFlags aSrcStageMask,
	VkPipelineStageFlags aDstStageMask, VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask)
{
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = aBuffer->getRawHandle();
	barrier.srcAccessMask = aSrcAccessMask;
	barrier.dstAccessMask = aDstAccessMask;
	barrier.size = aBuffer->getSize();
	mDevice->vkCmd.PipelineBarrier(mCommandBuffer,
		aSrcStageMask,
		aDstStageMask,
		0,
		0, VK_NULL_HANDLE,
		1, &barrier,
		0, VK_NULL_HANDLE);
}

void CommandBuffer::cmdBeginRendering(const std::vector<rvk::RenderInfo>& aColorAttachments, const rvk::RenderInfoDepth&& aDepthAttachments) const
{
	VkRenderingInfoKHR ri = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	ri.renderArea.extent.width = aColorAttachments.front().image->getExtent().width;
	ri.renderArea.extent.height = aColorAttachments.front().image->getExtent().height;
	ri.layerCount = 1;

	
	std::vector<VkRenderingAttachmentInfo> ca;
	ca.reserve(aColorAttachments.size());
	for (const auto& aColorAttachment : aColorAttachments)
	{
		VkRenderingAttachmentInfo rai = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		rai.imageView = aColorAttachment.image->getImageView();
		rai.imageLayout = aColorAttachment.image->getLayout();
		rai.clearValue.color = aColorAttachment.clearValue;
		rai.loadOp = aColorAttachment.loadOp;
		rai.storeOp = aColorAttachment.storeOp;
		ca.push_back(rai);
	}
	ri.colorAttachmentCount = static_cast<uint32_t>(aColorAttachments.size());
	ri.pColorAttachments = ri.colorAttachmentCount ? ca.data() : nullptr;

	
	VkRenderingAttachmentInfo depthRai = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	if (aDepthAttachments.image)
	{
		depthRai.imageView = aDepthAttachments.image->getImageView();
		depthRai.imageLayout = aDepthAttachments.image->getLayout();
		depthRai.clearValue.depthStencil = aDepthAttachments.clearValue;
		depthRai.loadOp = aDepthAttachments.loadOp;
		depthRai.storeOp = aDepthAttachments.storeOp;
		ri.pDepthAttachment = &depthRai;
	}
	mDevice->vkCmd.BeginRenderingKHR(mCommandBuffer, &ri);
}
void CommandBuffer::cmdEndRendering() const
{
	mDevice->vkCmd.EndRenderingKHR(mCommandBuffer);
}

SingleTimeCommand::SingleTimeCommand(CommandPool* aCommandPool, Queue* aQueue) :
mCommandPool(aCommandPool), mQueue(aQueue), mCommandBuffer(nullptr)
{
}

SingleTimeCommand::~SingleTimeCommand()
{
	if (mCommandBuffer) end();
}

void SingleTimeCommand::begin()
{
	if (mCommandBuffer) end();
	mCommandBuffer = mCommandPool->allocCommandBuffers(1).front();
	mCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

CommandBuffer* SingleTimeCommand::buffer() const
{
	return mCommandBuffer;
}

void SingleTimeCommand::end()
{
	mCommandBuffer->end();
	mQueue->submitCommandBuffers({ mCommandBuffer });
	mQueue->waitIdle();
	mCommandPool->freeCommandBuffers({ mCommandBuffer });
	mCommandBuffer = nullptr;
}

void SingleTimeCommand::execute(const std::function<void(CommandBuffer*)>& aFunction)
{
	begin();
	aFunction(mCommandBuffer);
	end();
}
