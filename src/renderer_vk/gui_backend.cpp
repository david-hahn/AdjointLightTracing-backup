#include <tamashii/renderer_vk/gui_backend.hpp>
#include <tamashii/renderer_vk/vulkan_instance.hpp>
#include <imgui.h>
#include <sys/stat.h>

#if defined( _WIN32 )
#include <imgui_impl_win32.h>
#elif defined( __APPLE__ )
#include <imgui_impl_osx.h>

#elif defined( __linux__ )
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )

#elif defined( VK_USE_PLATFORM_XCB_KHR )
#include <tamashii/core/gui/x11/imgui_impl_x11.h>
#endif
#endif

T_USE_NAMESPACE
RVK_USE_NAMESPACE

namespace {
	
	
	/*
	#version 450 core
	layout(location = 0) in vec2 aPos;
	layout(location = 1) in vec2 aUV;
	layout(location = 2) in vec4 aColor;
	layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

	out gl_PerVertex { vec4 gl_Position; };
	layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

	void main()
	{
		Out.Color = aColor;
		Out.UV = aUV;
		gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
	}
	*/
	const std::vector<uint32_t> __glsl_shader_vert_spv =
	{
		0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
		0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
		0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
		0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
		0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
		0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
		0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
		0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
		0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
		0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
		0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
		0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
		0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
		0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
		0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
		0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
		0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
		0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
		0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
		0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
		0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
		0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
		0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
		0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
		0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
		0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
		0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
		0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
		0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
		0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
		0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
		0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
		0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
		0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
		0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
		0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
		0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
		0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
		0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
		0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
		0x0000002d,0x0000002c,0x000100fd,0x00010038
	};

	
	
	/*
	#version 450 core
	layout(location = 0) out vec4 fColor;
	layout(set=0, binding=0) uniform sampler2D sTexture;
	layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
	void main()
	{
		fColor = In.Color * texture(sTexture, In.UV.st);
	}
	*/
	static const std::vector<uint32_t> __glsl_shader_frag_spv =
	{
		0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
		0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
		0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
		0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
		0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
		0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
		0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
		0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
		0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
		0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
		0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
		0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
		0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
		0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
		0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
		0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
		0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
		0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
		0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
		0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
		0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
		0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
		0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
		0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
		0x00010038
	};
}

GuiBackend::GuiBackend(VulkanInstance& aInstance, void* aSurfaceHandle,
	uint32_t aFrameCount, std::string_view aFontFilePath) : mInstance{ aInstance }, mDevice{ *aInstance.mDevice }, mCommandPool{ *aInstance.mSwapchainData.mCommandPool },  mSurfaceHandle {
	aSurfaceHandle
},
	mFrameCount{ aFrameCount }
{
	mImguiData = std::make_shared<Data>(mDevice, mFrameCount);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	
	io.IniFilename = nullptr;

	
	

	constexpr float scaleFactor = 1.1f;
	ImGui::StyleColorsDark();
	ImGui::GetStyle().ScaleAllSizes(scaleFactor);

	
	struct stat buffer {};
	if (stat(aFontFilePath.data(), &buffer) == 0) {
		if (!io.Fonts->AddFontFromFileTTF(aFontFilePath.data(), 13 * scaleFactor))
		{
			spdlog::error("failed to load ImGui font");
		}
	}

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	mImguiData->mImage.createImage2D(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	mImguiData->mImage.setSampler({ VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
		-1000, 1000 });

	SingleTimeCommand stc = { &mCommandPool, mDevice.getQueue(0, 0) };
	mImguiData->mImage.STC_UploadData2D(&stc, width, height, 4, pixels);
	mImguiData->mImage.STC_TransitionImage(&stc, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	mImguiData->mDescriptor.addImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	mImguiData->mDescriptor.setImage(0, &mImguiData->mImage);
	mImguiData->mDescriptor.finish();

	
	io.Fonts->SetTexID(mImguiData->mDescriptor.getHandle());

	mImguiData->mShader.addStage(Shader::Stage::VERTEX, __glsl_shader_vert_spv);
	mImguiData->mShader.addStage(Shader::Stage::FRAGMENT, __glsl_shader_frag_spv);
	mImguiData->mShader.finish();

	_buildPipeline();

	void* surfaceHandle = mSurfaceHandle;
#ifdef WIN32
	ImGui_ImplWin32_Init(const_cast<void*>(surfaceHandle));
#elif defined( __APPLE__ )
	ImGui_ImplOSX_Init((NSView*)surfaceHandle);
#elif defined( __linux__ )
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )

#elif defined( VK_USE_PLATFORM_XCB_KHR )
	ImGui_ImplX11_Init(*((xcb_window_t*)surfaceHandle));
#endif
#endif
}

GuiBackend::~GuiBackend()
{
	mImguiData.reset();
#ifdef WIN32
	ImGui_ImplWin32_Shutdown();
#elif defined( __APPLE__ )
	ImGui_ImplOSX_Shutdown();
#elif defined( __linux__ )
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )

#elif defined( VK_USE_PLATFORM_XCB_KHR )
	ImGui_ImplX11_Shutdown();
#endif
#endif
	ImGui::DestroyContext();
}

void GuiBackend::prepare(uint32_t aWidth, uint32_t aHeight)
{
#ifdef WIN32
	ImGui_ImplWin32_NewFrame();
#elif defined( __APPLE__ )
	ImGui::GetIO().DisplaySize = ImVec2{ static_cast<float>(aWidth), static_cast<float>(aHeight) };
	ImGui_ImplOSX_NewFrame(nullptr);
#elif defined( __linux__ )
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )

#elif defined( VK_USE_PLATFORM_XCB_KHR )
	ImGui_ImplX11_NewFrame();
#endif
#endif
	ImGui::NewFrame();
}

void GuiBackend::draw(rvk::CommandBuffer* cb, const uint32_t cFrameIdx) const
{
	ImGui::Render();

	Data* id = mImguiData.get();
	const ImDrawData* draw_data = ImGui::GetDrawData();
	const int fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	const int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0) return;

	if (draw_data->TotalVtxCount > 0)
	{
		const size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		const size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
		if (id->mVertex[cFrameIdx].getSize() < vertex_size) {
			id->mVertex[cFrameIdx].destroy();
			id->mVertex[cFrameIdx].create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		if (id->mIndex[cFrameIdx].getSize() < index_size) {
			id->mIndex[cFrameIdx].destroy();
			id->mIndex[cFrameIdx].create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, index_size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}

		
		id->mVertex[cFrameIdx].mapBuffer();
		id->mIndex[cFrameIdx].mapBuffer();
		auto* vtx_dst = reinterpret_cast<ImDrawVert*>(id->mVertex[cFrameIdx].getMemoryPointer());
		auto* idx_dst = reinterpret_cast<ImDrawIdx*>(id->mIndex[cFrameIdx].getMemoryPointer());
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtx_dst += cmd_list->VtxBuffer.Size;
			idx_dst += cmd_list->IdxBuffer.Size;
		}
		VkMappedMemoryRange range[2] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = id->mVertex[cFrameIdx].getMemory();
		range[0].size = VK_WHOLE_SIZE;
		range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[1].memory = id->mIndex[cFrameIdx].getMemory();
		range[1].size = VK_WHOLE_SIZE;
		mDevice.vk.FlushMappedMemoryRanges(mDevice.getHandle(), 2, &range[0]);
		id->mVertex[cFrameIdx].unmapBuffer();
		id->mIndex[cFrameIdx].unmapBuffer();
	}

	id->mPipeline.CMD_SetViewport(cb);
	id->mPipeline.CMD_SetScissor(cb);
	id->mPipeline.CMD_BindDescriptorSets(cb, { &id->mDescriptor });
	id->mPipeline.CMD_BindPipeline(cb);

	id->mVertex[cFrameIdx].CMD_BindVertexBuffer(cb, 0, 0);
	id->mIndex[cFrameIdx].CMD_BindIndexBuffer(cb, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

	float scale[2];
	scale[0] = 2.0f / draw_data->DisplaySize.x;
	scale[1] = 2.0f / draw_data->DisplaySize.y;
	float translate[2];
	translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
	translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
	id->mPipeline.CMD_SetPushConstant(cb, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, &scale);
	id->mPipeline.CMD_SetPushConstant(cb, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, &translate);

	
	
	
	const ImVec2 clip_off = draw_data->DisplayPos;         
	const ImVec2 clip_scale = draw_data->FramebufferScale; 
	uint32_t global_vtx_offset = 0;
	uint32_t global_idx_offset = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			
			ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
			ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

			
			if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
			if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
			if (clip_max.x > static_cast<float>(fb_width)) { clip_max.x = static_cast<float>(fb_width); }
			if (clip_max.y > static_cast<float>(fb_height)) { clip_max.y = static_cast<float>(fb_height); }
			if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
				continue;

			
			VkRect2D scissor;
			scissor.offset.x = static_cast<int32_t>(clip_min.x);
			scissor.offset.y = static_cast<int32_t>(clip_min.y);
			scissor.extent.width = static_cast<uint32_t>(clip_max.x - clip_min.x);
			scissor.extent.height = static_cast<uint32_t>(clip_max.y - clip_min.y);
			mDevice.vkCmd.SetScissor(cb->getHandle(), 0, 1, &scissor);

			id->mPipeline.CMD_BindDescriptorSets(cb, { &id->mDescriptor });
			
			const VkDescriptorSet desc_set[1] = { static_cast<VkDescriptorSet>(pcmd->TextureId) };
			mDevice.vkCmd.BindDescriptorSets(cb->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, id->mPipeline.getLayout(), 0, 1, &desc_set[0], 0, nullptr);
			
			id->mPipeline.CMD_DrawIndexed(cb, pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, static_cast<int32_t>(pcmd->VtxOffset + global_vtx_offset));
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}
}

void GuiBackend::_buildPipeline() const
{
	mImguiData->mPipeline.destroy();
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	rvk::RPipeline::global_render_state.polygonMode = VK_POLYGON_MODE_FILL;
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_NONE;
	rvk::RPipeline::global_render_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rvk::RPipeline::global_render_state.dynamicStates.viewport = true;
	rvk::RPipeline::global_render_state.dynamicStates.scissor = true;
	rvk::RPipeline::global_render_state.colorFormat = { mInstance.mSwapchainData.mFrames.front().mColor->getFormat() };
	rvk::RPipeline::global_render_state.depthFormat = mInstance.mSwapchainData.mFrames.front().mDepth->getFormat();
	rvk::RPipeline::global_render_state.renderpass = VK_NULL_HANDLE;
	VkPipelineColorBlendAttachmentState color_attachment{};
	color_attachment.blendEnable = VK_TRUE;
	color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	rvk::RPipeline::global_render_state.colorBlend = { color_attachment };
	mImguiData->mPipeline.setShader(&mImguiData->mShader);
	mImguiData->mPipeline.addPushConstant(rvk::Shader::Stage::VERTEX, 0, sizeof(float) * 4);
	mImguiData->mPipeline.addDescriptorSet({ &mImguiData->mDescriptor });
	mImguiData->mPipeline.addBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX);
	mImguiData->mPipeline.addAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, IM_OFFSETOF(ImDrawVert, pos));
	mImguiData->mPipeline.addAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, IM_OFFSETOF(ImDrawVert, uv));
	mImguiData->mPipeline.addAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, IM_OFFSETOF(ImDrawVert, col));
	mImguiData->mPipeline.finish();
	rvk::RPipeline::popRenderState();
}
