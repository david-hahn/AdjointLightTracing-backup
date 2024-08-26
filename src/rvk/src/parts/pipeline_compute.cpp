#include <rvk/parts/pipeline_compute.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

CPipeline::CPipeline(LogicalDevice* aDevice) : Pipeline(aDevice, VK_PIPELINE_BIND_POINT_COMPUTE), mShader(nullptr) {}

rvk::CPipeline::~CPipeline()
{
    mShader = nullptr;
}
void rvk::CPipeline::destroy()
{
    Pipeline::destroy();
    if (mShader != nullptr) {
        mShader->unlinkPipeline(this);
        mShader = nullptr;
    }
}

void rvk::CPipeline::setShader(CShader* const aShader)
{
    mShader = aShader;
    
    aShader->linkPipeline(this);
}

void CPipeline::CMD_Dispatch(const CommandBuffer* aCmdBuffer, const uint32_t aGroupCountX, const uint32_t aGroupCountY,
                             const uint32_t aGroupCountZ) const
{
    mDevice->vkCmd.Dispatch(aCmdBuffer->getHandle(), aGroupCountX, aGroupCountY, aGroupCountZ);
}

/**
* PRIVATE
**/
void rvk::CPipeline::createPipeline(const bool aFront) {
    VkComputePipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.stage = mShader->getShaderStageCreateInfo()[0];
    createInfo.layout = mLayout;
    VK_CHECK(mDevice->vk.CreateComputePipelines(mDevice->getHandle(), mCache, 1, &createInfo, NULL, aFront ? &mHandleFront : &mHandleBack), " failed to create Pipeline!");
}