#include <rvk/parts/pipeline.hpp>
#include <rvk/parts/command_buffer.hpp>
#include "rvk_private.hpp"

#include <fstream>

RVK_USE_NAMESPACE

VkPipelineCache rvk::Pipeline::mCache = VK_NULL_HANDLE;
bool rvk::Pipeline::pcache = false;
std::string rvk::Pipeline::cache_dir;
std::vector<unsigned char> pipeline_cache;

/**
* PIPELINE
**/

Pipeline::Pipeline(LogicalDevice* aDevice, const VkPipelineBindPoint aPipelineBindPoint) : mDevice(aDevice),
                                                                                           mPipelineBindPoint(aPipelineBindPoint),
                                                                                           mLayout(VK_NULL_HANDLE),
                                                                                           mHandleFront(VK_NULL_HANDLE),
                                                                                           mHandleBack(VK_NULL_HANDLE)
{
    mDescriptors.reserve(3);
    mPushConstantRange.reserve(3);
}

rvk::Pipeline::~Pipeline()
{
    destroy();
}

Pipeline::Pipeline(const Pipeline& aOther) : mDevice(aOther.mDevice), mPipelineBindPoint(aOther.mPipelineBindPoint),
mLayout(VK_NULL_HANDLE), mHandleFront(VK_NULL_HANDLE), mHandleBack(VK_NULL_HANDLE),
mDescriptors(aOther.mDescriptors), mPushConstantRange(aOther.mPushConstantRange)
{}

Pipeline& Pipeline::operator=(const Pipeline& aOther)
{
    if (this != &aOther) {
        mDevice = aOther.mDevice;
        mDescriptors = aOther.mDescriptors;
        mPushConstantRange = aOther.mPushConstantRange;
    }
    return *this;
}

void rvk::Pipeline::addDescriptorSet(const std::vector<Descriptor*>& aDescriptors) {
    for (Descriptor* d : aDescriptors) {
        mDescriptors.push_back(d);
    }
}
void rvk::Pipeline::addPushConstant(const VkShaderStageFlags aStageFlags, const uint32_t aOffset, const uint32_t aSize) {
	const VkPushConstantRange range{ aStageFlags, aOffset, aSize };
    mPushConstantRange.push_back(range);
}
void rvk::Pipeline::createPipelineCache() const
{
#ifdef NDEBUG
    if (rvk::Pipeline::pcache && rvk::Pipeline::mCache == VK_NULL_HANDLE) {
        VkPipelineCacheCreateInfo pipelineCacheInfo = {};
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        
        std::ifstream input;
        input.open(rvk::Pipeline::cache_dir + "/pipeline_cache.bin", std::ios::in | std::ios::binary);
        pipeline_cache = std::vector<unsigned char>(std::istreambuf_iterator<char>(input), {});
        if (input.is_open()) {
            pipelineCacheInfo.initialDataSize = pipeline_cache.size();
            pipelineCacheInfo.pInitialData = pipeline_cache.data();
            input.close();
        }

        VK_CHECK(mDevice->vk.CreatePipelineCache(mDevice->getHandle(), &pipelineCacheInfo, VK_NULL_HANDLE, &mCache), "failed to create Pipeline Cache!");
    }
#endif
}
void rvk::Pipeline::createPipelineLayout() {
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.setLayoutCount = mDescriptors.size();
    std::vector<VkDescriptorSetLayout> dLayout(pipelineLayoutInfo.setLayoutCount);
    for (uint32_t i = 0; i < pipelineLayoutInfo.setLayoutCount; i++) {
        dLayout[i] = mDescriptors[i]->mLayout;
    }

    pipelineLayoutInfo.pSetLayouts = toArrayPointer(dLayout);
    pipelineLayoutInfo.pushConstantRangeCount = mPushConstantRange.size();
    pipelineLayoutInfo.pPushConstantRanges = toArrayPointer(mPushConstantRange);

    VK_CHECK(mDevice->vk.CreatePipelineLayout(mDevice->getHandle(), &pipelineLayoutInfo, VK_NULL_HANDLE, &mLayout), "failed to create Pipeline Layout!");
}

void rvk::Pipeline::rebuildPipeline()
{
    const std::lock_guard lock(mBackHandleWriteMutex);
    
    if (mHandleBack) {
        mDevice->vk.DestroyPipeline(mDevice->getHandle(), mHandleBack, VK_NULL_HANDLE);
        mHandleBack = VK_NULL_HANDLE;
    }
    createPipeline(false);
}
void rvk::Pipeline::finish()
{
    createPipelineCache();
    createPipelineLayout();
    createPipeline(true);

#ifdef NDEBUG
    if (Pipeline::pcache) {
        size_t pcache_count = 0;
        VK_CHECK(mDevice->vk.GetPipelineCacheData(mDevice->getHandle(), mCache, &pcache_count, NULL), "failed to get Pipeline Cache Size!");
        
        std::vector<unsigned char> pcache_data(pcache_count);
        VK_CHECK(mDevice->vk.GetPipelineCacheData(mDevice->getHandle(), rvk::Pipeline::mCache, &pcache_count, pcache_data.data()), "failed to get Pipeline Cache Data!");

        if (pipeline_cache.size() != pcache_count) {
            pipeline_cache = pcache_data;
            
            
            std::ofstream output;
            output.open(Pipeline::cache_dir + "/pipeline_cache.bin", std::ios::out | std::ios::binary);
            if (output.is_open()) {
                output.write(reinterpret_cast<char*>(pcache_data.data()), pcache_data.size());
                output.close();
            }
        }
    }
#endif

}

void rvk::Pipeline::destroy()
{
    if (mHandleFront) {
        mDevice->vk.DestroyPipeline(mDevice->getHandle(), mHandleFront, VK_NULL_HANDLE);
        mHandleFront = VK_NULL_HANDLE;
    }
    if (mHandleBack) {
        mDevice->vk.DestroyPipeline(mDevice->getHandle(), mHandleBack, VK_NULL_HANDLE);
        mHandleBack = VK_NULL_HANDLE;
    }
    if (mLayout) {
        mDevice->vk.DestroyPipelineLayout(mDevice->getHandle(), mLayout, VK_NULL_HANDLE);
        mLayout = VK_NULL_HANDLE;
    }
    /*if (cache) {
        vkDestroyPipelineCache(vk.device, cache, NULL);
        cache = VK_NULL_HANDLE;
    }*/
    mDescriptors.clear();
    mPushConstantRange.clear();
    for (const VkPipeline p : mRipPipelines) if (p) mDevice->vk.DestroyPipeline(mDevice->getHandle(), p, VK_NULL_HANDLE);
    mRipPipelines.clear();
}

VkPipelineLayout Pipeline::getLayout() const
{
    return mLayout;
}

void Pipeline::CMD_BindPipeline(const CommandBuffer* aCmdBuffer)
{
    
    const std::unique_lock<std::mutex> lock(mBackHandleWriteMutex, std::try_to_lock);
    if (lock.owns_lock() && mHandleBack != VK_NULL_HANDLE) {
        mRipPipelines.push_back(mHandleFront);
        mHandleFront = mHandleBack;
        mHandleBack = VK_NULL_HANDLE;
    }
    mDevice->vkCmd.BindPipeline(aCmdBuffer->getHandle(), mPipelineBindPoint, mHandleFront);
}

void Pipeline::CMD_BindDescriptorSets(const CommandBuffer* aCmdBuffer, const std::vector<Descriptor*>& aDescriptors,
                                      const uint32_t aFirstSetIndex) const
{
    std::vector<VkDescriptorSet> sets(aDescriptors.size());
    for (uint32_t i = 0; i < aDescriptors.size(); i++) sets[i] = aDescriptors[i]->mSet;
    mDevice->vkCmd.BindDescriptorSets(aCmdBuffer->getHandle(), mPipelineBindPoint, mLayout, aFirstSetIndex,
        static_cast<uint32_t>(aDescriptors.size()), toArrayPointer(sets), 0, VK_NULL_HANDLE);
}

void Pipeline::CMD_SetPushConstant(const CommandBuffer* aCmdBuffer, const VkShaderStageFlags aStage, const uint32_t aOffset,
                                   const uint32_t aSize, const void* aData) const
{
    mDevice->vkCmd.PushConstants(aCmdBuffer->getHandle(), mLayout, aStage, aOffset, aSize, aData);
}
