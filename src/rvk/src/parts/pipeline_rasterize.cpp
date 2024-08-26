#include <rvk/parts/pipeline_rasterize.hpp>
#include <rvk/parts/command_buffer.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE
rvk::RPipeline::render_state_s	rvk::RPipeline::global_render_state;
std::deque<rvk::RPipeline::render_state_s> rvk::RPipeline::mRenderStateStack;

RPipeline::RPipeline(LogicalDevice* aDevice) : Pipeline(aDevice, VK_PIPELINE_BIND_POINT_GRAPHICS), mLocalRenderState(), mShader(nullptr)
{
    mBindingDescription.reserve(3);
    mAttributeDescription.reserve(3);
}

rvk::RPipeline::~RPipeline()
{
    mBindingDescription.clear();
    mAttributeDescription.clear();
    if (mShader != nullptr) {
        mShader->unlinkPipeline(this);
        mShader = nullptr;
    }
}

RPipeline::RPipeline(const RPipeline& aOther) : Pipeline(aOther), mLocalRenderState(), mShader(aOther.mShader),
                                                   mBindingDescription(aOther.mBindingDescription),
                                                   mAttributeDescription(aOther.mAttributeDescription)
{
    mShader->linkPipeline(this);
}

RPipeline& RPipeline::operator=(const RPipeline& aOther)
{
    if (this != &aOther) {
        Pipeline::operator=(aOther);
        mShader = aOther.mShader;
        mShader->linkPipeline(this);
        mBindingDescription = aOther.mBindingDescription;
        mAttributeDescription = aOther.mAttributeDescription;
    }
    return *this;
}

void rvk::RPipeline::setShader(RShader* const aShader) {
    if (mShader != nullptr) mShader->unlinkPipeline(this);
    mShader = aShader;
    
    aShader->linkPipeline(this);
}
void rvk::RPipeline::addBindingDescription(const uint32_t aBinding, const uint32_t aStride, const VkVertexInputRate aInputRate) {
	const VkVertexInputBindingDescription desc{ aBinding, aStride, aInputRate };
    mBindingDescription.push_back(desc);
}

void rvk::RPipeline::addAttributeDescription(const uint32_t aBinding, const uint32_t aLocation, const VkFormat aFormat, const uint32_t aOffset) {
	const VkVertexInputAttributeDescription desc{ aLocation, aBinding, aFormat, aOffset };
    mAttributeDescription.push_back(desc);
}

void rvk::RPipeline::destroy()
{
    Pipeline::destroy();
    mBindingDescription.clear();
    mAttributeDescription.clear();
    mShader = nullptr;
}

void RPipeline::CMD_SetViewport(const CommandBuffer* aCmdBuffer, const float aWidth, const float aHeight, const float aX, const float aY,
                                const float aMinDepth, const float aMaxDepth) const
{
    const VkViewport viewport = { aX,aY,aWidth,aHeight,aMinDepth,aMaxDepth };
    mDevice->vkCmd.SetViewport(aCmdBuffer->getHandle(), 0, 1, &viewport);
}

void RPipeline::CMD_SetViewport(const CommandBuffer* aCmdBuffer, const std::vector<VkViewport>& aViewports, const uint32_t aFirst) const
{
    mDevice->vkCmd.SetViewport(aCmdBuffer->getHandle(), aFirst, static_cast<uint32_t>(aViewports.size()), aViewports.data());
}

void RPipeline::CMD_SetViewport(const CommandBuffer* aCmdBuffer) const
{
    mDevice->vkCmd.SetViewport(aCmdBuffer->getHandle(), 0, 1, &global_render_state.viewport);
}

void RPipeline::CMD_SetScissor(const CommandBuffer* aCmdBuffer, const uint32_t aWidth, const uint32_t aHeight, const int32_t aX, const int32_t aY) const
{
    const VkRect2D rect = { {aX, aY}, {aWidth, aHeight} };
    mDevice->vkCmd.SetScissor(aCmdBuffer->getHandle(), 0, 1, &rect);
}

void RPipeline::CMD_SetScissor(const CommandBuffer* aCmdBuffer, const std::vector<VkRect2D>& aScissors, const uint32_t aFirst) const
{
    mDevice->vkCmd.SetScissor(aCmdBuffer->getHandle(), aFirst, static_cast<uint32_t>(aScissors.size()), aScissors.data());
}

void RPipeline::CMD_SetScissor(const CommandBuffer* aCmdBuffer) const
{
    mDevice->vkCmd.SetScissor(aCmdBuffer->getHandle(), 0, 1, &global_render_state.scissor);
}

void RPipeline::CMD_SetDepthBias(const CommandBuffer* aCmdBuffer, const float aConstantFactor, const float aSlopeFactor, const float aClamp) const
{
    mDevice->vkCmd.SetDepthBias(aCmdBuffer->getHandle(), aConstantFactor, aClamp, aSlopeFactor);
}

void RPipeline::CMD_SetDepthBias(const CommandBuffer* aCmdBuffer) const
{
    mDevice->vkCmd.SetDepthBias(aCmdBuffer->getHandle(), global_render_state.depth_bias.constantFactor, global_render_state.depth_bias.clamp, global_render_state.depth_bias.slopeFactor);
}

void RPipeline::CMD_Draw(const CommandBuffer* aCmdBuffer, const uint32_t aCount, const uint32_t aFirstVertex) const
{
    mDevice->vkCmd.Draw(aCmdBuffer->getHandle(), aCount, 1, aFirstVertex, 0);
}

void RPipeline::CMD_DrawIndexed(const CommandBuffer* aCmdBuffer, const uint32_t aCount, const uint32_t aFirstIndex, const int32_t aVertexOffset) const
{
    mDevice->vkCmd.DrawIndexed(aCmdBuffer->getHandle(), aCount, 1, aFirstIndex, aVertexOffset, 0);
}

void RPipeline::CMD_DrawIndirect(const CommandBuffer* aCmdBuffer, const BufferHandle aBufferHandle, const uint32_t aStride,
    const uint32_t aCount) const
{
    mDevice->vkCmd.DrawIndirect(aCmdBuffer->getHandle(), aBufferHandle.mBuffer->getRawHandle(), aBufferHandle.mBufferOffset, aCount, aStride);
}

void RPipeline::CMD_DrawIndexedIndirect(const CommandBuffer* aCmdBuffer, const BufferHandle aBufferHandle, const uint32_t aStride,
    const uint32_t aCount) const
{
    mDevice->vkCmd.DrawIndexedIndirect(aCmdBuffer->getHandle(), aBufferHandle.mBuffer->getRawHandle(), aBufferHandle.mBufferOffset, aCount, aStride);
}

void RPipeline::pushRenderState()
{
    rvk::RPipeline::mRenderStateStack.push_back(rvk::RPipeline::global_render_state);
}

void RPipeline::popRenderState()
{
    if (!rvk::RPipeline::mRenderStateStack.empty()) {
        rvk::RPipeline::global_render_state = rvk::RPipeline::mRenderStateStack.back();
        rvk::RPipeline::mRenderStateStack.pop_back();
    }
}

RPipeline::render_state_s* RPipeline::getLocalRenderState()
{ return &mLocalRenderState; }

/**
* PRIVATE
**/
void rvk::RPipeline::createPipeline(const bool aFront) {
    
    if (aFront) mLocalRenderState = RPipeline::global_render_state;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(mBindingDescription.size());
    vertexInputInfo.pVertexBindingDescriptions = toArrayPointer(mBindingDescription);
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(mAttributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = toArrayPointer(mAttributeDescription);

    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    
    pipelineInfo.stageCount = static_cast<uint32_t>(mShader->getShaderStageCreateInfo().size());
    pipelineInfo.pStages = mShader->getShaderStageCreateInfo().data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia = {};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = mLocalRenderState.primitiveTopology;
    ia.primitiveRestartEnable = VK_FALSE;
    pipelineInfo.pInputAssemblyState = &ia;

    VkPipelineViewportStateCreateInfo vp = {};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.pViewports = &mLocalRenderState.viewport;
    vp.viewportCount = 1;
    vp.pScissors = &mLocalRenderState.scissor;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs = {};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = mLocalRenderState.polygonMode;
    rs.cullMode = mLocalRenderState.cullMode;
    rs.frontFace = mLocalRenderState.frontFace;
    rs.lineWidth = 1.0f;
    rs.depthBiasEnable = VK_TRUE;
    rs.depthBiasConstantFactor = mLocalRenderState.depth_bias.constantFactor;
    rs.depthBiasClamp = mLocalRenderState.depth_bias.clamp;
    rs.depthBiasSlopeFactor = mLocalRenderState.depth_bias.slopeFactor;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms = {};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.minSampleShading = 1.0f;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineColorBlendStateCreateInfo cb = {};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = mLocalRenderState.colorBlend.size();
    cb.pAttachments = toArrayPointer(mLocalRenderState.colorBlend);
    pipelineInfo.pColorBlendState = &cb;

    mLocalRenderState.dsBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineInfo.pDepthStencilState = &mLocalRenderState.dsBlend;

    std::vector<VkDynamicState> dynStates;
    dynStates.reserve(3);
    if (mLocalRenderState.dynamicStates.viewport) dynStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    if (mLocalRenderState.dynamicStates.scissor) dynStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    if (mLocalRenderState.dynamicStates.depth_bias) dynStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    
    VkPipelineDynamicStateCreateInfo dyn = {};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dyn.pDynamicStates = dynStates.empty() ? nullptr : dynStates.data();
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = mLayout;

    VkPipelineRenderingCreateInfo prci = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    if(mLocalRenderState.renderpass) pipelineInfo.renderPass = mLocalRenderState.renderpass;
    else {
        prci.colorAttachmentCount = mLocalRenderState.colorFormat.size();
        prci.pColorAttachmentFormats = mLocalRenderState.colorFormat.data();
        prci.depthAttachmentFormat = mLocalRenderState.depthFormat;
        pipelineInfo.pNext = &prci;
    }
    VK_CHECK(mDevice->vk.CreateGraphicsPipelines(mDevice->getHandle(), mCache, 1, &pipelineInfo, nullptr, aFront ? &mHandleFront : &mHandleBack), " failed to create Pipeline!");
}
