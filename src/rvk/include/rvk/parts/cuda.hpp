#pragma once
#include <rvk/parts/rvk_public.hpp>

#include <unordered_map>
#include <string>
RVK_BEGIN_NAMESPACE
class LogicalDevice;
class CommandBuffer;
#ifdef VK_NVX_binary_import
/**
* CUDA (VK_NVX_binary_import)
**/
class Cuda {
public:
	enum class Source {
		CU,	
		CUBIN
	};

													Cuda();
													Cuda(LogicalDevice* aDevice);
													~Cuda();

	void											loadModule(Source aSource, const std::string& aFilePath);
	void											createFunction(const std::string& aName);

	void											destroy();

	void											CMD_LaunchKernel(const CommandBuffer* aCmdBuffer, const std::string& aFunction,
	                                                                 uint32_t aGridDimX, uint32_t aGridDimY, uint32_t aGridDimZ,
	                                                                 uint32_t aBlockDimX, uint32_t aBlockDimY, uint32_t aBlockDimZ,
	                                                                 uint32_t aSharedMemBytes, const std::vector<void*>&
	                                                                 aParams) const;
private:
	LogicalDevice*									mDevice;
	VkCuModuleNVX									mCuModule;
	std::unordered_map<std::string, VkCuFunctionNVX>mCuFunctions;
};
#endif

RVK_END_NAMESPACE