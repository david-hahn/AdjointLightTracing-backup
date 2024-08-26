#include <rvk/parts/shader_compute.hpp>
#include <rvk/parts/rvk_public.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

bool rvk::CShader::checkStage(const Stage aStage) {
	const bool supported = (aStage == Stage::COMPUTE);
    ASSERT(supported, "Stage not supported by Compute Shader");
    ASSERT(this->mStageData.size() == 0, "Compute Shader can only have one stage");
    return supported && (this->mStageData.size() == 0);
}