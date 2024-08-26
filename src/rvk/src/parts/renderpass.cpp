#include <rvk/parts/renderpass.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

rvk::AttachmentDescription::AttachmentDescription(const VkFormat aFormat, const VkImageLayout aInit, const VkImageLayout aFinal) : 
    attachment({ 0, aFormat, VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
    aInit, aFinal }) {
}

void rvk::AttachmentDescription::setAttachmentOps(const VkAttachmentLoadOp aLoad, const VkAttachmentStoreOp aStore)
{
    attachment.loadOp = aLoad;
    attachment.storeOp = aStore;
}

void rvk::AttachmentDescription::setAttachmentStencilOps(const VkAttachmentLoadOp aLoad, const VkAttachmentStoreOp aStore)
{
    attachment.stencilLoadOp = aLoad;
    attachment.stencilStoreOp = aStore;
}

Renderpass::Renderpass(LogicalDevice* aDevice) : mDevice(aDevice), mRenderPass(VK_NULL_HANDLE)
{
}

rvk::Renderpass::~Renderpass() {
    destroy();
}

void Renderpass::reserve(const int aAttachmentsReserve, const int aSubpassesReserve, const int aDependenciesReserve)
{
    mAttachments.reserve(aAttachmentsReserve);
    mSubpasses.reserve(aSubpassesReserve);
    mDependencies.reserve(aDependenciesReserve);
}

void rvk::Renderpass::destroy()
{
    if (mRenderPass) {
        mDevice->vk.DestroyRenderPass(mDevice->getHandle(), mRenderPass, nullptr);
        mRenderPass = nullptr;
    }
    mAttachments.clear();
    mDependencies.clear();
    mSubpasses.clear();
}

VkRenderPass Renderpass::getHandle() const
{
    return mRenderPass;
}

void rvk::Renderpass::addAttachment(const AttachmentDescription& aAttachmentDescription)
{
    mAttachments.push_back(aAttachmentDescription.attachment);
}

void rvk::Renderpass::addSubpass(const VkPipelineBindPoint aPipelineBindPoint, const int aColorAttachmentReserveCount)
{
    mSubpasses.push_back({ aPipelineBindPoint, {}, {} });
    mSubpasses.back().colorAttachmentRefs.reserve(aColorAttachmentReserveCount);
}

void rvk::Renderpass::setColorAttachment(const int aSubpassIdx, const int aAttachmentIdx, const VkImageLayout aDuring)
{
    VkAttachmentReference attachmentRef = {};
    attachmentRef.attachment = aAttachmentIdx;
    attachmentRef.layout = aDuring;
    mSubpasses[aSubpassIdx].colorAttachmentRefs.push_back(attachmentRef);
}

void rvk::Renderpass::setDepthStencilAttachment(const int aSubpassIdx, const int aAttachmentIdx, const VkImageLayout aDuring)
{
    if (!mSubpasses[aSubpassIdx].depthAttachmentRef.has_value()) mSubpasses[aSubpassIdx].depthAttachmentRef.reset();
    VkAttachmentReference attachmentRef = {};
    attachmentRef.attachment = aAttachmentIdx;
    attachmentRef.layout = aDuring;
    mSubpasses[aSubpassIdx].depthAttachmentRef = std::optional<VkAttachmentReference>{ attachmentRef };
}


void rvk::Renderpass::addDependency(const uint32_t aSrcSubpass, const uint32_t aDstSubpass, const VkPipelineStageFlags aSrcStageMask, const VkPipelineStageFlags aDstStageMask, const VkAccessFlags aSrcAccessMask, const VkAccessFlags aDstAccessMask, const VkDependencyFlags aDependencyFlag)
{
	const VkSubpassDependency dependency = { aSrcSubpass, aDstSubpass, aSrcStageMask, aDstStageMask, aSrcAccessMask, aDstAccessMask, aDependencyFlag };
    mDependencies.push_back(dependency);
}

void Renderpass::buildSubpassDescriptions()
{
    mSubpassDescriptions.clear();
    mSubpassDescriptions.reserve(mSubpasses.size());

    for (Subpass_s& s : mSubpasses) {
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = s.pipelineBindPoint;
        subpass.colorAttachmentCount = static_cast<uint32_t>(s.colorAttachmentRefs.size());
        subpass.pColorAttachments = toArrayPointer(s.colorAttachmentRefs);
        subpass.pDepthStencilAttachment = s.depthAttachmentRef.has_value() ? &s.depthAttachmentRef.value() : nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = VK_NULL_HANDLE;
        mSubpassDescriptions.push_back(subpass);
    }
}

void rvk::Renderpass::finish()
{
    buildSubpassDescriptions();

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(mAttachments.size());
    renderPassInfo.pAttachments = toArrayPointer(mAttachments);
    renderPassInfo.subpassCount = static_cast<uint32_t>(mSubpassDescriptions.size());
    renderPassInfo.pSubpasses = toArrayPointer(mSubpassDescriptions);
    renderPassInfo.dependencyCount = static_cast<uint32_t>(mDependencies.size());
    renderPassInfo.pDependencies = toArrayPointer(mDependencies);

    VK_CHECK(mDevice->vk.CreateRenderPass(mDevice->getHandle(), &renderPassInfo, nullptr, &mRenderPass), "failed to create Renderpass");
}

