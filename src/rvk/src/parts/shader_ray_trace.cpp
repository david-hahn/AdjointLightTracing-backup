#include <rvk/parts/shader_ray_trace.hpp>
#include <rvk/parts/rvk_public.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

bool rvk::RTShader::checkStage(const Shader::Stage aStage) {
	const bool supported = (aStage == Stage::RAYGEN || aStage == Stage::ANY_HIT || aStage == Stage::CLOSEST_HIT || aStage == Stage::MISS || aStage == Stage::INTERSECTION);
    ASSERT(supported, "Stage not supported by Ray Tracing Shader");
    return supported;
}

bool rvk::RTShader::checkShaderGroupOrderGeneral(const Shader::Stage aNewStage) {
    int last_index = -1;
    for (int i = 0; i < static_cast<int>(mShaderGroupCreateInfos.size()); i++) {
	    const VkRayTracingShaderGroupCreateInfoKHR& info = mShaderGroupCreateInfos[i];
        if (aNewStage == Stage::RAYGEN && info.generalShader != VK_SHADER_UNUSED_KHR && mStageData[info.generalShader].stage == Stage::RAYGEN) last_index = i;
        if (aNewStage == Stage::MISS && info.generalShader != VK_SHADER_UNUSED_KHR && mStageData[info.generalShader].stage == Stage::MISS) last_index = i;
        if (aNewStage == Stage::CALLABLE && info.generalShader != VK_SHADER_UNUSED_KHR && mStageData[info.generalShader].stage == Stage::CALLABLE) last_index = i;
    }
    if (last_index == -1) return true;
    if (static_cast<int>(mShaderGroupCreateInfos.size()) != (last_index + 1)) return false;
    return true;
}

bool rvk::RTShader::checkShaderGroupOrderHit()
{
    int last_index = -1;
    for (int i = 0; i < static_cast<int>(mShaderGroupCreateInfos.size()); i++) {
        VkRayTracingShaderGroupCreateInfoKHR& info = mShaderGroupCreateInfos[i];
        if (info.anyHitShader != VK_SHADER_UNUSED_KHR || info.closestHitShader != VK_SHADER_UNUSED_KHR || info.intersectionShader != VK_SHADER_UNUSED_KHR) last_index = i;
    }
    if (last_index == -1) return true;
    if (static_cast<int>(mShaderGroupCreateInfos.size()) != (last_index + 1)) return false;
    return true;
}

RTShader::RTShader(LogicalDevice* aDevice) : Shader(aDevice), mShaderGroupCreateInfos({}), mCurrentBaseAlignedOffset(0),
	mRgen(sbtInfo()), mMiss(sbtInfo()), mHit(sbtInfo()), mCallable(sbtInfo())
{}

void RTShader::reserveGroups(const int aCount)
{ mShaderGroupCreateInfos.reserve(aCount); }


void rvk::RTShader::addGeneralShaderGroup(const int aGeneralShaderIndex)
{
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtp = mDevice->getPhysicalDevice()->getRayTracingPipelineProperties();
    ASSERT(aGeneralShaderIndex != -1, "General Shader not found");

    
    const bool correctOrder = checkShaderGroupOrderGeneral(mStageData[aGeneralShaderIndex].stage);
    ASSERT(correctOrder, "Arrange ray tracing shader groups in bundles of RAYGEN - MISS - CLOSEST-HIT/ANY-HIT/INTERSECTION - CALLABLE");
    if (!correctOrder) Logger::warning("Bad ray tracing shader groups order");

    
    if (mStageData[aGeneralShaderIndex].stage == Stage::RAYGEN && mRgen.baseAlignedOffset == -1) {
        mRgen.countOffset = mShaderGroupCreateInfos.size();
        mCurrentBaseAlignedOffset = mRgen.baseAlignedOffset = rountUpToMultipleOf<int>(mCurrentBaseAlignedOffset, rtp.shaderGroupBaseAlignment);
    }
    if (mStageData[aGeneralShaderIndex].stage == Stage::MISS && mMiss.baseAlignedOffset == -1) {
        mMiss.countOffset = mShaderGroupCreateInfos.size();
        mCurrentBaseAlignedOffset = mMiss.baseAlignedOffset = rountUpToMultipleOf<int>(mCurrentBaseAlignedOffset, rtp.shaderGroupBaseAlignment);
    }
    if (mStageData[aGeneralShaderIndex].stage == Stage::CALLABLE && mCallable.baseAlignedOffset == -1) {
        mCallable.countOffset = mShaderGroupCreateInfos.size();
        mCurrentBaseAlignedOffset = mCallable.baseAlignedOffset = rountUpToMultipleOf<int>(mCurrentBaseAlignedOffset, rtp.shaderGroupBaseAlignment);
    }
    const int handleSizeAlignment = rountUpToMultipleOf<int>(rtp.shaderGroupHandleSize, rtp.shaderGroupHandleAlignment);
    if (mStageData[aGeneralShaderIndex].stage == Stage::RAYGEN) {
        mCurrentBaseAlignedOffset += rountUpToMultipleOf<int>(handleSizeAlignment, rtp.shaderGroupBaseAlignment);
        mRgen.handleSize += rountUpToMultipleOf<int>(handleSizeAlignment, rtp.shaderGroupBaseAlignment);
        mRgen.count++;
    }
    if (mStageData[aGeneralShaderIndex].stage == Stage::MISS) {
        mCurrentBaseAlignedOffset += handleSizeAlignment;
        mMiss.handleSize += handleSizeAlignment;
        mMiss.count++;
    }
    if (mStageData[aGeneralShaderIndex].stage == Stage::CALLABLE) {
        mCurrentBaseAlignedOffset += handleSizeAlignment;
        mCallable.handleSize += handleSizeAlignment;
        mCallable.count++;
    }

    
    VkRayTracingShaderGroupCreateInfoKHR sgCreateInfo = {};
    sgCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    sgCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    sgCreateInfo.closestHitShader = sgCreateInfo.anyHitShader = sgCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
    sgCreateInfo.generalShader = aGeneralShaderIndex != -1 ? aGeneralShaderIndex : VK_SHADER_UNUSED_KHR;
    mShaderGroupCreateInfos.push_back(sgCreateInfo);
}
void rvk::RTShader::addGeneralShaderGroup(const std::string& aGeneralShader) {
	const int generalShaderIndex = findShaderIndex(aGeneralShader);
    addGeneralShaderGroup(generalShaderIndex);
}
void rvk::RTShader::addHitShaderGroup(const int aClosestHitShaderIndex, const int aAnyHitShaderIndex)
{
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtp = mDevice->getPhysicalDevice()->getRayTracingPipelineProperties();
    
    const bool correct_order = checkShaderGroupOrderHit();
    ASSERT(correct_order, "Arrange ray tracing shader groups in bundles of RAYGEN - MISS - CLOSEST-HIT/ANY-HIT/INTERSECTION - CALLABLE");
    if (!correct_order) Logger::warning("Bad ray tracing shader groups order");

    
    if (mHit.baseAlignedOffset == -1) {
        mHit.countOffset = mShaderGroupCreateInfos.size();
        mCurrentBaseAlignedOffset = mHit.baseAlignedOffset = rountUpToMultipleOf<int>(mCurrentBaseAlignedOffset, rtp.shaderGroupBaseAlignment);
    }
    const int handleSizeAlignment = rountUpToMultipleOf<int>(rtp.shaderGroupHandleSize, rtp.shaderGroupHandleAlignment);
    mCurrentBaseAlignedOffset += handleSizeAlignment;
    mHit.handleSize += handleSizeAlignment;
    mHit.count++;

    
    VkRayTracingShaderGroupCreateInfoKHR sgCreateInfo = {};
    sgCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    sgCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    sgCreateInfo.generalShader = sgCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
    sgCreateInfo.closestHitShader = aClosestHitShaderIndex != -1 ? aClosestHitShaderIndex : VK_SHADER_UNUSED_KHR;
    sgCreateInfo.anyHitShader = aAnyHitShaderIndex != -1 ? aAnyHitShaderIndex : VK_SHADER_UNUSED_KHR;
    mShaderGroupCreateInfos.push_back(sgCreateInfo);
}
void rvk::RTShader::addHitShaderGroup(const std::string& aClosestHitShader, const std::string& aAnyHitShader) {
    int closestHitShaderIndex = -1;
    if (!aClosestHitShader.empty()) {
        closestHitShaderIndex = findShaderIndex(aClosestHitShader);
        ASSERT(closestHitShaderIndex != -1, "Closest Hit Shader not found");
    }
    int anyHitShaderIndex = -1;
    if (!aAnyHitShader.empty()) {
        anyHitShaderIndex = findShaderIndex(aAnyHitShader);
        ASSERT(anyHitShaderIndex != -1, "Any Hit Shader not found");
    }
    addHitShaderGroup(closestHitShaderIndex, anyHitShaderIndex);
}
void rvk::RTShader::addProceduralShaderGroup(const std::string& aClosestHitShader, const std::string& aAnyHitShader, const std::string& aIntersectionShader)
{
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtp = mDevice->getPhysicalDevice()->getRayTracingPipelineProperties();
	const int closestHitShaderIndex = findShaderIndex(aClosestHitShader);
	const int anyHitShaderIndex = findShaderIndex(aAnyHitShader);
	const int intersectionShaderIndex = findShaderIndex(aIntersectionShader);
    ASSERT(aClosestHitShader.empty() || closestHitShaderIndex != -1, "Closest Hit Shader not found");
    ASSERT(aAnyHitShader.empty() || anyHitShaderIndex != -1, "Any Hit Shader not found");
    ASSERT(aIntersectionShader.empty() || intersectionShaderIndex != -1, "Intersection Shader not found");

    
	const bool correctOrder = checkShaderGroupOrderHit();
    ASSERT(correctOrder, "Arrange ray tracing shader groups in bundles of RAYGEN - MISS - CLOSEST-HIT/ANY-HIT/INTERSECTION - CALLABLE");
    if (!correctOrder) Logger::warning("Bad ray tracing shader groups order");

    
    if (mHit.baseAlignedOffset == -1) {
        mHit.countOffset = mShaderGroupCreateInfos.size();
        mCurrentBaseAlignedOffset = mHit.baseAlignedOffset = rountUpToMultipleOf<int>(mCurrentBaseAlignedOffset, rtp.shaderGroupBaseAlignment);
    }
    const int handleSizeAlignment = rountUpToMultipleOf<int>(rtp.shaderGroupHandleSize, rtp.shaderGroupHandleAlignment);
    mCurrentBaseAlignedOffset += handleSizeAlignment;
    mHit.handleSize += handleSizeAlignment;
    mHit.count++;

    
    VkRayTracingShaderGroupCreateInfoKHR sgCreateInfo = {};
    sgCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    sgCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
    sgCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
    sgCreateInfo.closestHitShader = closestHitShaderIndex != -1 ? closestHitShaderIndex : VK_SHADER_UNUSED_KHR;
    sgCreateInfo.anyHitShader = anyHitShaderIndex != -1 ? anyHitShaderIndex : VK_SHADER_UNUSED_KHR;
    sgCreateInfo.intersectionShader = intersectionShaderIndex != -1 ? intersectionShaderIndex : VK_SHADER_UNUSED_KHR;
    mShaderGroupCreateInfos.push_back(sgCreateInfo);
}

void rvk::RTShader::destroy()
{
    Shader::destroy();
    mShaderGroupCreateInfos.clear();
    mShaderGroupCreateInfos = {};
    mCurrentBaseAlignedOffset = 0;
    mRgen = {};
    mMiss = {};
    mHit = {};
    mCallable = {};
}

std::vector<VkRayTracingShaderGroupCreateInfoKHR>& RTShader::getShaderGroupCreateInfo()
{ return mShaderGroupCreateInfos; }

RTShader::sbtInfo& RTShader::getRgenSBTInfo()
{ return mRgen; }

RTShader::sbtInfo& RTShader::getMissSBTInfo()
{ return mMiss; }

RTShader::sbtInfo& RTShader::getHitSBTInfo()
{ return mHit; }

RTShader::sbtInfo& RTShader::getCallableSBTInfo()
{ return mCallable; }
