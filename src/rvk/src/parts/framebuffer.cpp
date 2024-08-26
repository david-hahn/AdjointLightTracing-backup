#include <rvk/parts/framebuffer.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

Framebuffer::Framebuffer(LogicalDevice* aDevice) : mDevice(aDevice), mRenderPass(nullptr), mExtent(), mFramebuffer(VK_NULL_HANDLE)
{
}

rvk::Framebuffer::~Framebuffer() {
	destroy();
}

void Framebuffer::setRenderpass(Renderpass* aRenderpass)
{
	mRenderPass = aRenderpass;
}

Renderpass* Framebuffer::getRenderpass() const
{
	return mRenderPass;
};

void rvk::Framebuffer::destroy() {
	if (mFramebuffer) {
		mDevice->vk.DestroyFramebuffer(mDevice->getHandle(), mFramebuffer, nullptr);
		mFramebuffer = nullptr;
	}
	mRenderPass = nullptr;
	mAttachments.clear();
	mExtent = {};
}

VkFramebuffer Framebuffer::getHandle() const
{
	return mFramebuffer;
}

void Framebuffer::addAttachment(Image* aImage)
{
	const VkExtent3D e = aImage->getExtent();
	if (mAttachments.empty()) mExtent = { e.width, e.height };
	else
	{
		ASSERT(mExtent.width == e.width && mExtent.height == e.height, "All framebuffer attachments need to be the same size!");
	}
	mAttachments.push_back(aImage);
}

Image* Framebuffer::getAttachment(const int aIndex) const
{
	return mAttachments[aIndex];
};

void rvk::Framebuffer::finish()
{
	std::vector<VkImageView> views;
	views.reserve(mAttachments.size());
	for (const Image* attachment : mAttachments) {
		views.push_back(attachment->mImageView);
	}
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = mRenderPass->mRenderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
	framebufferInfo.pAttachments = toArrayPointer(views);
	framebufferInfo.width = mExtent.width;
	framebufferInfo.height = mExtent.height;
	framebufferInfo.layers = 1;

	VK_CHECK(mDevice->vk.CreateFramebuffer(mDevice->getHandle(), &framebufferInfo, NULL, &this->mFramebuffer), "failed to create Framebuffer");
}


void Framebuffer::CMD_BeginRenderClear(const CommandBuffer* aCmdBuffer, const std::vector<VkClearValue>& aClearValues) const
{
	VkRenderPassBeginInfo rpBeginInfo = {};
	rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBeginInfo.renderPass = mRenderPass->mRenderPass;
	rpBeginInfo.framebuffer = mFramebuffer;
	rpBeginInfo.renderArea.extent.width = mExtent.width;
	rpBeginInfo.renderArea.extent.height = mExtent.height;
	rpBeginInfo.clearValueCount = static_cast<uint32_t>(aClearValues.size());
	rpBeginInfo.pClearValues = aClearValues.empty() ? nullptr : aClearValues.data();

	mDevice->vkCmd.BeginRenderPass(aCmdBuffer->getHandle(), &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Framebuffer::CMD_EndRender(const CommandBuffer* aCmdBuffer) const
{
	for (uint32_t i = 0; i < mAttachments.size(); i++)
	{
		mAttachments[i]->mLayout = mRenderPass->mAttachments[i].finalLayout;
	}
	mDevice->vkCmd.EndRenderPass(aCmdBuffer->getHandle());
}

