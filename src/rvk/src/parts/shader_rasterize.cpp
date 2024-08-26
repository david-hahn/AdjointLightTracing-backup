#include <rvk/parts/shader_rasterize.hpp>
#include <rvk/parts/rvk_public.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

bool rvk::RShader::checkStage(const Shader::Stage aStage) {
	const bool supported = (aStage == Stage::VERTEX || aStage == Stage::TESS_CONTROL || aStage == Stage::TESS_EVALUATION || aStage == Stage::GEOMETRY || aStage == Stage::FRAGMENT);
    ASSERT(supported, "Stage not supported by Rasterizer Shader");
    return supported;
}