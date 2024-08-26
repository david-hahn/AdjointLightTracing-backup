#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class CommandBuffer;
class Image;
class Renderpass;
class Framebuffer {
public:
													Framebuffer(LogicalDevice* aDevice);
													~Framebuffer();

													
	void											setRenderpass(Renderpass* aRenderpass);
	Renderpass*										getRenderpass() const;

													
													
	void											addAttachment(Image* aImage);
	Image*											getAttachment(int aIndex) const;

	void											finish();
	void											destroy();

	VkFramebuffer									getHandle() const;

	void											CMD_BeginRenderClear(const CommandBuffer* aCmdBuffer, const std::vector<VkClearValue>& aClearValues = {}) const;
	void											CMD_EndRender(const CommandBuffer* aCmdBuffer) const;
private:
	friend class									Swapchain;
	LogicalDevice*									mDevice;
	Renderpass*										mRenderPass;
	VkExtent2D										mExtent;
	std::vector<Image*>								mAttachments;
	VkFramebuffer									mFramebuffer;
};
RVK_END_NAMESPACE