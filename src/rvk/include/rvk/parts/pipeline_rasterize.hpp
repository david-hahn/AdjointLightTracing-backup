#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/pipeline.hpp>

RVK_BEGIN_NAMESPACE
class RShader;
class Buffer;
class RPipeline final : public Pipeline {
public:
													RPipeline(LogicalDevice* aDevice);
													~RPipeline();
													RPipeline(const RPipeline& aOther);
	RPipeline&										operator=(const RPipeline& aOther);
													
	void											setShader(RShader* aShader);
	void											addBindingDescription(uint32_t aBinding, uint32_t aStride, VkVertexInputRate aInputRate);
	void											addAttributeDescription(uint32_t aBinding, uint32_t aLocation, VkFormat aFormat, uint32_t aOffset);
	void											destroy() override;

													
													
													
	void											CMD_SetViewport(const CommandBuffer* aCmdBuffer, float aWidth, float aHeight, float aX = 0, float aY = 0, float aMinDepth = 0, float aMaxDepth = 1) const;
	void											CMD_SetViewport(const CommandBuffer* aCmdBuffer, const std::vector<VkViewport>& aViewports, uint32_t aFirst = 0) const;
	void											CMD_SetViewport(const CommandBuffer* aCmdBuffer) const;
	void											CMD_SetScissor(const CommandBuffer* aCmdBuffer, uint32_t aWidth, uint32_t aHeight, int32_t aX = 0, int32_t aY = 0) const;
	void											CMD_SetScissor(const CommandBuffer* aCmdBuffer, const std::vector<VkRect2D>& aScissors, uint32_t aFirst = 0) const;
	void											CMD_SetScissor(const CommandBuffer* aCmdBuffer) const;
	void											CMD_SetDepthBias(const CommandBuffer* aCmdBuffer, float aConstantFactor, float aSlopeFactor, float aClamp = 0) const;
	void											CMD_SetDepthBias(const CommandBuffer* aCmdBuffer) const;
	void											CMD_Draw(const CommandBuffer* aCmdBuffer, uint32_t aCount, uint32_t aFirstVertex = 0) const;
	void											CMD_DrawIndexed(const CommandBuffer* aCmdBuffer, uint32_t aCount, uint32_t aFirstIndex = 0, int32_t aVertexOffset = 0) const;
	void											CMD_DrawIndirect(const CommandBuffer* aCmdBuffer, BufferHandle aBufferHandle, uint32_t aStride, uint32_t aCount) const;
	void											CMD_DrawIndexedIndirect(const CommandBuffer* aCmdBuffer, BufferHandle aBufferHandle, uint32_t aStride, uint32_t aCount) const;

													
													
	struct render_state_s {
		struct dynamic_states_s {
			bool									viewport;
			bool									scissor;
			bool									depth_bias;
		};
		dynamic_states_s							dynamicStates;
		VkPolygonMode                               polygonMode = VK_POLYGON_MODE_FILL;
		VkPrimitiveTopology							primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkFrontFace                                 frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkViewport									viewport = { 0,0,0,0,0,1 };
		VkRect2D									scissor = {};
		struct depth_bias_s
		{
			float                                   constantFactor;
			float                                   clamp;
			float                                   slopeFactor;
		};
		depth_bias_s								depth_bias;
		VkCullModeFlags                             cullMode = VK_CULL_MODE_NONE;
		std::vector<VkPipelineColorBlendAttachmentState> colorBlend = { {} };
		VkPipelineDepthStencilStateCreateInfo		dsBlend = {};
		struct render_pass_s
		{
			
		};
		VkRenderPass								renderpass;
		std::vector<VkFormat>						colorFormat;
		VkFormat									depthFormat;
	};
	static void										pushRenderState();
	static void										popRenderState();
	static render_state_s							global_render_state;

	render_state_s*									getLocalRenderState();
private:
	void											createPipeline(bool aFront) override;

	render_state_s									mLocalRenderState;

	rvk::RShader*									mShader;
	std::vector<VkVertexInputBindingDescription>	mBindingDescription;
	std::vector<VkVertexInputAttributeDescription>	mAttributeDescription;
	static std::deque<render_state_s>				mRenderStateStack;
};
RVK_END_NAMESPACE