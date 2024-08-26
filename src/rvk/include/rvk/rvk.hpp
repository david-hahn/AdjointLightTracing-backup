#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/logger.hpp>

#include <rvk/parts/instance.hpp>
#include <rvk/parts/device_physical.hpp>
#include <rvk/parts/device_logical.hpp>
#include <rvk/parts/swapchain.hpp>
#include <rvk/parts/command_pool.hpp>
#include <rvk/parts/command_buffer.hpp>
#include <rvk/parts/queue.hpp>
#include <rvk/parts/queue_family.hpp>
#include <rvk/parts/fence.hpp>

#include <rvk/parts/buffer.hpp>
#include <rvk/parts/sampler.hpp>
#include <rvk/parts/image.hpp>
#include <rvk/parts/acceleration_structure.hpp>
#include <rvk/parts/acceleration_structure_bottom_level.hpp>
#include <rvk/parts/acceleration_structure_top_level.hpp>
#include <rvk/parts/descriptor.hpp>

#include <rvk/parts/pipeline.hpp>
#include <rvk/parts/pipeline_compute.hpp>
#include <rvk/parts/pipeline_rasterize.hpp>
#include <rvk/parts/pipeline_ray_trace.hpp>

#include <rvk/parts/framebuffer.hpp>
#include <rvk/parts/renderpass.hpp>

#include <rvk/parts/shader.hpp>
#include <rvk/parts/shader_compute.hpp>
#include <rvk/parts/shader_rasterize.hpp>
#include <rvk/parts/shader_ray_trace.hpp>

#include <rvk/parts/cuda.hpp>

RVK_BEGIN_NAMESPACE

namespace memory {
	
	VkPipelineStageFlags							getSrcStageMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout);
	VkPipelineStageFlags							getDstStageMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout);
	VkAccessFlags									getSrcAccessMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout);
	VkAccessFlags									getDstAccessMask(VkImageLayout aOldImageLayout, VkImageLayout aNewImageLayout);
}
namespace image
{
	void											CMD_CopyImageToImage(CommandBuffer* cb, rvk::Image* src_image, rvk::Image* dst_image);
}
namespace swapchain {
	
	std::vector<uint8_t> 							readPixelsScreen(SingleTimeCommand* aStc, LogicalDevice* aDevice, Image* aSrcImage, bool aAlpha);
	void											CMD_CopyImageToCurrentSwapchainImage(VkCommandBuffer cb, rvk::Image* src_image);
	void											CMD_BlitImageToImage(CommandBuffer* cb, rvk::Image* aSrcImage, rvk::Image* aDstImage, VkFilter aFilter);
}

namespace error {
    const char*										errorString(VkResult aErrorCode);
}

constexpr void VK_CHECK(const VkResult aResult, const std::string& aString)
{
	if (aResult < 0) {
		Logger::error("Vulkan: " + aString + " " + rvk::error::errorString(aResult));
	}
}


#define RVK_EXPAND(x) x 
#define RVK_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N 
#define RVK_NARG(...) RVK_EXPAND(RVK_ARG_N(__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0))

#define RVK_CHAIN_1_(V) RVK_CHAIN_1 V
#define RVK_CHAIN_2_(V) RVK_CHAIN_2 V
#define RVK_CHAIN_3_(V) RVK_CHAIN_3 V
#define RVK_CHAIN_4_(V) RVK_CHAIN_4 V
#define RVK_CHAIN_5_(V) RVK_CHAIN_5 V
#define RVK_CHAIN_6_(V) RVK_CHAIN_6 V
#define RVK_CHAIN_7_(V) RVK_CHAIN_7 V
#define RVK_CHAIN_8_(V) RVK_CHAIN_8 V
#define RVK_CHAIN_9_(V) RVK_CHAIN_9 V
#define RVK_CHAIN_10_(V) RVK_CHAIN_10 V
#define RVK_CHAIN_11_(V) RVK_CHAIN_11 V
#define RVK_CHAIN_12_(V) RVK_CHAIN_12 V
#define RVK_CHAIN_13_(V) RVK_CHAIN_13 V
#define RVK_CHAIN_14_(V) RVK_CHAIN_14 V
#define RVK_CHAIN_15_(V) RVK_CHAIN_15 V

#define RVK_CHAIN_2(A,B) A.pNext = &B
#define RVK_CHAIN_3(_0,_1,_2) RVK_CHAIN_2(_0,_1); RVK_CHAIN_2(_1,_2)
#define RVK_CHAIN_4(_0,_1,_2,_3) RVK_CHAIN_3(_0,_1,_2); RVK_CHAIN_2(_2,_3)
#define RVK_CHAIN_5(_0,_1,_2,_3,_4) RVK_CHAIN_4(_0,_1,_2,_3); RVK_CHAIN_2(_3,_4)
#define RVK_CHAIN_6(_0,_1,_2,_3,_4,_5) RVK_CHAIN_5(_0,_1,_2,_3,_4); RVK_CHAIN_2(_4,_5)
#define RVK_CHAIN_7(_0,_1,_2,_3,_4,_5,_6) RVK_CHAIN_6(_0,_1,_2,_3,_4,_5); RVK_CHAIN_2(_5,_6)
#define RVK_CHAIN_8(_0,_1,_2,_3,_4,_5,_6,_7) RVK_CHAIN_7(_0,_1,_2,_3,_4,_5,_6); RVK_CHAIN_2(_6,_7)
#define RVK_CHAIN_9(_0,_1,_2,_3,_4,_5,_6,_7,_8) RVK_CHAIN_8(_0,_1,_2,_3,_4,_5,_6,_7); RVK_CHAIN_2(_7,_8)
#define RVK_CHAIN_10(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9) RVK_CHAIN_9(_0,_1,_2,_3,_4,_5,_6,_7,_8); RVK_CHAIN_2(_8,_9)
#define RVK_CHAIN_11(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) RVK_CHAIN_10(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9); RVK_CHAIN_2(_9,_10)
#define RVK_CHAIN_12(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) RVK_CHAIN_11(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10); RVK_CHAIN_2(_10,_11)
#define RVK_CHAIN_13(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) RVK_CHAIN_12(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11); RVK_CHAIN_2(_11,_12)
#define RVK_CHAIN_14(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) RVK_CHAIN_13(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12); RVK_CHAIN_2(_12,_13)
#define RVK_CHAIN_15(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) RVK_CHAIN_14(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13); RVK_CHAIN_2(_13,_14)
#define RVK_CHAIN_16(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) RVK_CHAIN_15(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14); RVK_CHAIN_2(_14,_15)

#define RVK_CHAIN__(N, ...)		RVK_CHAIN_ ## N ## _((__VA_ARGS__))
#define RVK_CHAIN_(N, ...)		RVK_CHAIN__(N, __VA_ARGS__)
#define rvkChain(...)			RVK_CHAIN_(RVK_NARG(__VA_ARGS__), __VA_ARGS__)

RVK_END_NAMESPACE
