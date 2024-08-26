#include <rvk/parts/cuda.hpp>
#include "rvk_private.hpp"
#include <fstream>
RVK_USE_NAMESPACE

#ifdef VK_NVX_binary_import
namespace {
    bool readFile(const std::string& aFilename, std::string& aFileContent) {
        std::ifstream file(aFilename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        const size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        aFileContent = std::string(buffer.begin(), buffer.end());
        return true;
    }
}

Cuda::Cuda(): Cuda(nullptr)
{
}

Cuda::Cuda(LogicalDevice* aDevice) : mDevice(aDevice), mCuModule(VK_NULL_HANDLE)
{
}

Cuda::~Cuda() {
    destroy();
}

void Cuda::loadModule(const Source aSource, const std::string& aFilePath) {
    std::string file_content;
    if (!readFile(aFilePath, file_content)) Logger::error("Could not load: " + aFilePath);

    VkCuModuleCreateInfoNVX createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_CU_MODULE_CREATE_INFO_NVX;
    createInfo.pData = file_content.c_str();
    createInfo.dataSize = file_content.size();
    VK_CHECK(mDevice->vk.CreateCuModuleNVX(mDevice->getHandle(), &createInfo, NULL, &mCuModule), "failed to create Cuda Module!");
}

void Cuda::createFunction(const std::string& aName)
{
    if (mCuFunctions.find(aName) != mCuFunctions.end()) return;
    VkCuFunctionCreateInfoNVX createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_CU_FUNCTION_CREATE_INFO_NVX;
    createInfo.module = mCuModule;
    createInfo.pName = aName.c_str();
    VkCuFunctionNVX	cuFunction;
    VK_CHECK(mDevice->vk.CreateCuFunctionNVX(mDevice->getHandle(), &createInfo, NULL, &cuFunction), "failed to create Cuda Function!");
    mCuFunctions.insert(std::pair<std::string, VkCuFunctionNVX>(aName, cuFunction));
}


void Cuda::destroy() {
    mDevice->vk.DestroyCuModuleNVX(mDevice->getHandle(), mCuModule, VK_NULL_HANDLE);
    for (const auto pair : mCuFunctions) {
        mDevice->vk.DestroyCuFunctionNVX(mDevice->getHandle(), pair.second, VK_NULL_HANDLE);
    }
    mCuFunctions.clear();
}

void Cuda::CMD_LaunchKernel(const CommandBuffer* aCmdBuffer, const std::string& aFunction, const uint32_t aGridDimX, const uint32_t aGridDimY,
                            const uint32_t aGridDimZ, const uint32_t aBlockDimX, const uint32_t aBlockDimY, const uint32_t aBlockDimZ, const uint32_t aSharedMemBytes,
                            const std::vector<void*>& aParams) const
{

    VkCuLaunchInfoNVX launchInfo = {};
    launchInfo.sType = VK_STRUCTURE_TYPE_CU_LAUNCH_INFO_NVX;
    try { launchInfo.function = mCuFunctions.at(aFunction); }
    catch (...) { Logger::warning("Vulkan: Cuda function '" + aFunction + "' not found"); return; }

    launchInfo.gridDimX = aGridDimX;
    launchInfo.gridDimY = aGridDimY;
    launchInfo.gridDimZ = aGridDimZ;

    launchInfo.blockDimX = aBlockDimX;
    launchInfo.blockDimY = aBlockDimY;
    launchInfo.blockDimZ = aBlockDimZ;

    launchInfo.sharedMemBytes = aSharedMemBytes;

    launchInfo.paramCount = aParams.size();
    launchInfo.pParams = aParams.data();

    mDevice->vkCmd.CuLaunchKernelNVX(aCmdBuffer->getHandle(), &launchInfo);
}
#endif
