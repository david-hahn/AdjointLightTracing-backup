#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class Pipeline;

class Shader {
public:
	enum class Source {
		SPV,		
		GLSL,		
		HLSL		
	};
	enum Stage {
		
		VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
		TESS_CONTROL = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		TESS_EVALUATION = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT,
		FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
		
		COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT,
		
		RAYGEN = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		ANY_HIT = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
		CLOSEST_HIT = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		MISS = VK_SHADER_STAGE_MISS_BIT_KHR,
		INTERSECTION = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
		CALLABLE = VK_SHADER_STAGE_CALLABLE_BIT_KHR
	};

													
	void											reserve(int aCount);
	
													
	void											addStage(Source aSource, Stage aStage, std::string_view aFilePath,
														const std::vector<std::string>& aDefines = {}, std::string_view aEntryPoint = "main");
	void											addStageFromString(Source aSource, Stage aStage, std::string_view aCode,
																		const std::vector<std::string>& aDefines = {},
																		std::string_view aEntryPoint = "main");
	void											addStage(Stage aStage, const std::vector<uint32_t>& aSPV, std::string_view aEntryPoint = "main");
													
	void											addConstant(uint32_t aIndex, uint32_t aId, uint32_t aSize, uint32_t aOffset = 0);
	void											addConstant(const std::string& aFile, uint32_t aId, uint32_t aSize, uint32_t aOffset = 0);
													
													
	void											setConstantData(uint32_t aIndex, const void* aData, uint32_t aSize);
	void											setConstantData(const std::string& aFile, const void* aData, uint32_t aSize);
													
	void											finish();
													
	void											reloadShader(int aIndex);
	void											reloadShader(const std::vector<int>& aIndices);
	void											reloadShader(const char* aFile);
	void											reloadShader(const std::vector<std::string>& aFiles);

	std::vector<VkPipelineShaderStageCreateInfo>&	getShaderStageCreateInfo();

	virtual void									destroy();

	void											linkPipeline(Pipeline* aPipeline);
	void											unlinkPipeline(const Pipeline* aPipeline);

	static std::string								cache_dir;
	static bool										cache;

protected:
													Shader(LogicalDevice* aDevice) : mDevice(aDevice), mStageData({}), mShaderStageCreateInfos({}) { {} }
													~Shader();

													
	int												findShaderIndex(const std::string& aFile) const;
													
	virtual bool									checkStage(Stage stage) = 0;

	struct stage_s {
		Source										source;													
		Stage										stage;													
		std::string									file_path;												
		std::vector<std::string>					defines;												
		std::string									entry_point;											
		std::vector<VkSpecializationMapEntry>		const_entry;											
		VkSpecializationInfo						const_info;
		std::string									spv;													
	};
													
	static bool										loadShaderFromFile(stage_s* aData);
	static bool										loadShaderFromString(stage_s* aData, std::string_view aCode);
													
													
	static void										configureStage(const LogicalDevice* aDevice, VkPipelineShaderStageCreateInfo* aCreateInfo, const stage_s* aData);

	LogicalDevice*									mDevice;
													
	std::vector<stage_s>							mStageData;								
	std::vector<VkPipelineShaderStageCreateInfo>	mShaderStageCreateInfos;					

													
	std::deque<Pipeline*>							mPipelines;
};
RVK_END_NAMESPACE