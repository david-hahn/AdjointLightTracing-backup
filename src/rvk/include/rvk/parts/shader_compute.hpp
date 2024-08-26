#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/shader.hpp>

RVK_BEGIN_NAMESPACE
class CShader final : public Shader {
public:
													CShader(LogicalDevice* aDevice) : Shader(aDevice) {}
													~CShader() = default;
private:
	bool											checkStage(Stage aStage) override;
};
RVK_END_NAMESPACE