#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/shader.hpp>

RVK_BEGIN_NAMESPACE
class RShader final : public Shader {
public:
													RShader(LogicalDevice* aDevice) : Shader(aDevice) {}
													~RShader() = default;
private:
	bool											checkStage(Stage aStage) override;
};
RVK_END_NAMESPACE