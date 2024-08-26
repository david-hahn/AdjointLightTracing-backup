#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/pipeline.hpp>

RVK_BEGIN_NAMESPACE
	class LogicalDevice;
class CShader;
class CommandBuffer;
class CPipeline final : public Pipeline {
public:
													CPipeline(LogicalDevice* aDevice);
													~CPipeline();
													
	void											setShader(CShader* aShader);
	void											destroy() override;

													
	void											CMD_Dispatch(const CommandBuffer* aCmdBuffer, uint32_t aGroupCountX, uint32_t aGroupCountY = 1, uint32_t aGroupCountZ = 1) const;

private:
	void											createPipeline(bool aFront) override;

	rvk::CShader*									mShader;
};
RVK_END_NAMESPACE