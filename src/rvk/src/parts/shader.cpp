#include <rvk/parts/shader.hpp>
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/pipeline.hpp>
#include <rvk/rvk.hpp>
#include "rvk_private.hpp"

#include <fstream>
#include <sstream>
#include "shader_compiler.hpp"
RVK_USE_NAMESPACE

bool rvk::Shader::cache = false;
std::string rvk::Shader::cache_dir;

namespace {
    bool readFile(const std::string_view aFilename, std::string& aFileContent) {
        std::ifstream file(aFilename.data(), std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        const std::streamsize fileSize = file.tellg();
        aFileContent.resize(fileSize);
        file.seekg(0);
        file.read(aFileContent.data(), fileSize);
        file.close();
        return true;
    }
    void createShaderModule(const LogicalDevice* aDevice, VkShaderModule* aHandle, std::string_view aCode)
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = aCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(aCode.data());
        VK_CHECK(aDevice->vk.CreateShaderModule(aDevice->getHandle(), &createInfo, VK_NULL_HANDLE, aHandle), "failed to create Shader Module!");
    }
}

void Shader::reserve(const int aCount)
{ mStageData.reserve(aCount); mShaderStageCreateInfos.reserve(aCount); }

void Shader::addStage(const Source aSource, const Stage aStage, std::string_view aFilePath,
                      const std::vector<std::string>& aDefines, std::string_view aEntryPoint){
    stage_s data = {};
    data.source = aSource;
    data.stage = aStage;
    data.file_path = aFilePath;
    data.defines = aDefines;
    data.entry_point = aEntryPoint;
    const bool success = loadShaderFromFile(&data);
    assert("Shader compilation error" && success);
    mStageData.emplace_back(data);
}

void Shader::addStageFromString(const Source aSource, const Stage aStage, std::string_view aCode,
    const std::vector<std::string>& aDefines, std::string_view aEntryPoint)
{
    stage_s data = {};
    data.source = aSource;
    data.stage = aStage;
    data.file_path = "";
    data.defines = aDefines;
    data.entry_point = aEntryPoint;
    const bool success = loadShaderFromString(&data, aCode);
    assert("Shader compilation error" && success);
    mStageData.emplace_back(data);
}

void Shader::addStage(Stage aStage, const std::vector<uint32_t>& aSPV, std::string_view aEntryPoint)
{
    stage_s data = {};
    data.source = Shader::Source::SPV;
    data.stage = aStage;
    data.file_path = "";
    data.entry_point = aEntryPoint;
    data.spv.resize(aSPV.size() * 4);
    std::memcpy(data.spv.data(), aSPV.data(), aSPV.size() * 4);
    mStageData.emplace_back(data);
}

void Shader::addConstant(const uint32_t aIndex, const uint32_t aId, const uint32_t aSize, const uint32_t aOffset)
{
    if (aIndex >= mStageData.size()) {
        Logger::error("rvk Shader: can not add constant to stage that is not present");
        return;
    }
    mStageData[aIndex].const_entry.push_back({ aId, aOffset, aSize });
    mStageData[aIndex].const_info.pMapEntries = mStageData[aIndex].const_entry.data();
    mStageData[aIndex].const_info.mapEntryCount = static_cast<uint32_t>(mStageData[aIndex].const_entry.size());
}

void Shader::addConstant(const std::string& aFile, const uint32_t aId, const uint32_t aSize, const uint32_t aOffset)
{
	const int index = findShaderIndex(aFile);
    if (index >= mStageData.size() || index == -1) {
        Logger::error("rvk Shader: can not add constant to stage that is not present");
        return;
    }
    addConstant(index, aId, aSize, aOffset);
}

void Shader::setConstantData(const uint32_t aIndex, const void* aData, const uint32_t aSize)
{
    if (aIndex >= mStageData.size() || aIndex == -1) {
        Logger::error("rvk Shader: can not set constant data to stage that is not present");
        return;
    }
    mStageData[aIndex].const_info.pMapEntries = mStageData[aIndex].const_entry.data();
    mStageData[aIndex].const_info.mapEntryCount = static_cast<uint32_t>(mStageData[aIndex].const_entry.size());
    mStageData[aIndex].const_info.pData = aData;
    mStageData[aIndex].const_info.dataSize = aSize;
}

void Shader::setConstantData(const std::string& aFile, const void* aData, const uint32_t aSize)
{
	const int index = findShaderIndex(aFile);
    if (static_cast<uint32_t>(index) >= mStageData.size()) {
        Logger::error("rvk Shader: can not set constant data to stage that is not present");
        return;
    }
    setConstantData(index, aData, aSize);
}

void Shader::finish()
{
    mShaderStageCreateInfos.resize(mStageData.size());
    int i = 0;
    for (stage_s& s : mStageData) {
        configureStage(mDevice, &mShaderStageCreateInfos[i], &s);
        i++;
    }
}

void Shader::reloadShader(const int aIndex)
{
    const std::vector v{ aIndex };
    reloadShader(v);
}

void Shader::reloadShader(const std::vector<int>& aIndices)
{
    for (const int index : aIndices) {
        if (index == -1) continue;
        if (mStageData[index].file_path.empty()) {
            Logger::info("It is not possible to reload a shader loaded from string");
            return;
        }
        Logger::info("Reloading Shader: " + mStageData[index].file_path);
        
        bool success = loadShaderFromFile(&mStageData[index]);
        
        mDevice->vk.DestroyShaderModule(mDevice->getHandle(), mShaderStageCreateInfos[index].module, nullptr);
        
        configureStage(mDevice, &mShaderStageCreateInfos[index], &mStageData[index]);
    }
    
    for (Pipeline* p : mPipelines) p->rebuildPipeline();
}

void Shader::reloadShader(const char* aFile)
{
    const std::vector<std::string> v{ aFile };
    reloadShader(v);
}

void Shader::reloadShader(const std::vector<std::string>& aFiles) {
    std::vector<int> indices;
    indices.reserve(aFiles.size());
    for (const std::string& file : aFiles) indices.push_back(findShaderIndex(file));
    reloadShader(indices);
}

std::vector<VkPipelineShaderStageCreateInfo>& Shader::getShaderStageCreateInfo()
{ return mShaderStageCreateInfos; }

void Shader::destroy()
{
    for (stage_s& s : mStageData) {
        s.defines.clear();
        s.file_path.clear();
        s.spv.clear();
    }

    mStageData.clear();
    mStageData = {};
    for (const VkPipelineShaderStageCreateInfo& i : mShaderStageCreateInfos) mDevice->vk.DestroyShaderModule(mDevice->getHandle(), i.module, VK_NULL_HANDLE);
    mShaderStageCreateInfos.clear();
    mShaderStageCreateInfos = {};
    mPipelines.clear();
    mPipelines = {};
}

void Shader::linkPipeline(Pipeline* aPipeline)
{
    if (std::find(mPipelines.begin(), mPipelines.end(), aPipeline) == mPipelines.end())
    {
        mPipelines.push_back(aPipeline);
    }
}

void Shader::unlinkPipeline(const Pipeline* aPipeline)
{
	const auto it = std::find(mPipelines.begin(), mPipelines.end(), aPipeline);
    if (it != mPipelines.end())
    {
        mPipelines.erase(it);
    }
}


rvk::Shader::~Shader()
{
    destroy();
}
int rvk::Shader::findShaderIndex(const std::string& aFile) const
{
    if (aFile.empty()) return -1;
    for (int i = 0; i < static_cast<int>(mStageData.size()); i++) {
        if (aFile == mStageData[i].file_path) return i;
    }
    return -1;
}

bool Shader::loadShaderFromFile(stage_s* const aData) {
    std::string code;
    if (!readFile(aData->file_path, code)) {
        Logger::error("Could not load: " + aData->file_path);
        return false;
    }
    return loadShaderFromString(aData, code);
}
bool Shader::loadShaderFromString(stage_s* aData, std::string_view aCode)
{
    
    if (aData->source == rvk::Shader::Source::GLSL || aData->source == rvk::Shader::Source::HLSL) {
        scomp::preprocess(aData->source, aData->stage, aData->file_path, aCode, aData->defines, aData->entry_point);
        std::ostringstream oss;
        oss << rvk::Shader::cache_dir << "/" << scomp::getHash() << ".spv";
        
        if (rvk::Shader::cache && readFile(oss.str(), aData->spv)) {
            scomp::cleanup();
            return true;
        }
        else {
            Logger::info("- Cache not found, compiling: " + aData->file_path);
            bool success = scomp::compile(aData->spv, rvk::Shader::cache, rvk::Shader::cache_dir);
            scomp::cleanup();
            return success;
        }
    }
    
    else if (aData->source == rvk::Shader::Source::SPV) aData->spv = aCode;
    return true;
}
void Shader::configureStage(const LogicalDevice* aDevice, VkPipelineShaderStageCreateInfo* const aCreateInfo, const stage_s* const aData)
{
    aCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    aCreateInfo->stage = static_cast<VkShaderStageFlagBits>(aData->stage);
    createShaderModule(aDevice, &aCreateInfo->module, aData->spv);
    aCreateInfo->pName = aData->entry_point.c_str();

    
    
    
    
    aCreateInfo->pSpecializationInfo = &aData->const_info;
}