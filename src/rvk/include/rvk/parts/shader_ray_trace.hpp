#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/shader.hpp>

RVK_BEGIN_NAMESPACE
class RTShader final : public Shader {
public:
	struct sbtInfo {
													sbtInfo() : handleSize(0), baseAlignedOffset(-1), countOffset(-1), count(0) {}
		int											handleSize;
		int											baseAlignedOffset;
		int											countOffset;
		int											count;
	};


													RTShader(LogicalDevice* aDevice);

													
	void											reserveGroups(int aCount);
	
													
													
	void											addGeneralShaderGroup(int aGeneralShaderIndex);
	void											addGeneralShaderGroup(const std::string& aGeneralShader);
													
													
	void											addHitShaderGroup(int aClosestHitShaderIndex, int aAnyHitShaderIndex = -1);
	void											addHitShaderGroup(const std::string& aClosestHitShader, const std::string& aAnyHitShader = "");
													
													
	void											addProceduralShaderGroup(const std::string& aClosestHitShader, const std::string& aAnyHitShader = "",
	                                                                         const std::string& aIntersectionShader = "");

	void											destroy() override;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR>& getShaderGroupCreateInfo();
	sbtInfo&										getRgenSBTInfo();
	sbtInfo&										getMissSBTInfo();
	sbtInfo&										getHitSBTInfo();
	sbtInfo&										getCallableSBTInfo();
private:
	bool											checkStage(Stage aStage) override;
													
													
	bool											checkShaderGroupOrderGeneral(Stage aNewStage);
	bool											checkShaderGroupOrderHit();

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> mShaderGroupCreateInfos;

	int												mCurrentBaseAlignedOffset;

	sbtInfo											mRgen;
	sbtInfo											mMiss;
	sbtInfo											mHit;
	sbtInfo											mCallable;
};
RVK_END_NAMESPACE