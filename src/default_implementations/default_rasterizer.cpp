#include <tamashii/implementations/default_rasterizer.hpp>
#include <imgui.h>

#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/platform/filewatcher.hpp>

#include "../../../assets/shader/default_rasterizer/defines.h"

T_USE_NAMESPACE
RVK_USE_NAMESPACE

constexpr uint32_t VK_GLOBAL_IMAGE_SIZE = 256;
constexpr uint32_t VK_MATERIAL_SIZE = (3 * 1024);
constexpr uint32_t VK_LIGHT_SIZE = 128;

constexpr VkFormat COLOR_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

void DefaultRasterizer::windowSizeChanged(const uint32_t aWidth, const uint32_t aHeight) {
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	for (uint32_t frameIndex = 0; frameIndex < mRoot.frameCount(); frameIndex++) {
		GpuData::GpuFrameData& frameData = mData->frameData[frameIndex];
		frameData.color.destroy();
		frameData.depth.destroy();

		frameData.color.createImage2D(aWidth, aHeight, COLOR_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::COLOR_ATTACHMENT);
		frameData.depth.createImage2D(aWidth, aHeight, DEPTH_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::DEPTH_STENCIL_ATTACHMENT);
		frameData.color.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		frameData.depth.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	stc.end();
}

void DefaultRasterizer::entityAdded(const Ref& aRef)
{}

void DefaultRasterizer::entityRemoved(const Ref& aRef)
{}

bool DefaultRasterizer::drawOnMesh(const DrawInfo* aDrawInfo) {
	return false;
}


void DefaultRasterizer::prepare(RenderInfo_s& aRenderInfo) {
	SingleTimeCommand stc = mRoot.singleTimeCommand();

	mGpuTd.prepare(rvk::Shader::Stage::FRAGMENT, VK_GLOBAL_IMAGE_SIZE);
	mGpuMd.prepare(rvk::Buffer::Use::STORAGE, VK_MATERIAL_SIZE, true);
	mGpuLd.prepare(rvk::Buffer::Use::STORAGE, VK_LIGHT_SIZE);

	mData = std::make_unique<GpuData>(mRoot.device, mRoot.frameCount());

	RPipeline::global_render_state.renderpass = nullptr;
	RPipeline::global_render_state.scissor.extent.width = aRenderInfo.targetSize.x;
	RPipeline::global_render_state.scissor.extent.height = aRenderInfo.targetSize.y;
	RPipeline::global_render_state.viewport.height = static_cast<float>(aRenderInfo.targetSize.y);
	RPipeline::global_render_state.viewport.width = static_cast<float>(aRenderInfo.targetSize.x);
	RPipeline::global_render_state.colorFormat = { COLOR_FORMAT };
	RPipeline::global_render_state.depthFormat = DEPTH_FORMAT;

	
	for (uint32_t frameIndex = 0; frameIndex < mRoot.frameCount(); frameIndex++) {
		GpuData::GpuFrameData& frameData = mData->frameData[frameIndex];

		frameData.color.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, COLOR_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::COLOR_ATTACHMENT);
		frameData.depth.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, DEPTH_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::DEPTH_STENCIL_ATTACHMENT);
		stc.begin();
		frameData.color.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		frameData.depth.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		stc.end();

		
		frameData.globalUniformBuffer.create(rvk::Buffer::Use::UNIFORM, sizeof(GlobalUbo), rvk::Buffer::Location::HOST_COHERENT);
		frameData.globalUniformBuffer.mapBuffer();

		
		
		frameData.globalDescriptor.addUniformBuffer(GLOBAL_DESC_UBO_BINDING, rvk::Shader::Stage::VERTEX | rvk::Shader::Stage::GEOMETRY | rvk::Shader::Stage::FRAGMENT);
		frameData.globalDescriptor.addStorageBuffer(GLOBAL_DESC_MATERIAL_BUFFER_BINDING, rvk::Shader::Stage::VERTEX | rvk::Shader::Stage::FRAGMENT);
		frameData.globalDescriptor.addStorageBuffer(GLOBAL_DESC_LIGHT_BUFFER_BINDING, rvk::Shader::Stage::VERTEX | rvk::Shader::Stage::FRAGMENT);
		frameData.globalDescriptor.setBuffer(GLOBAL_DESC_UBO_BINDING, &frameData.globalUniformBuffer);
		frameData.globalDescriptor.setBuffer(GLOBAL_DESC_MATERIAL_BUFFER_BINDING, mGpuMd.getMaterialBuffer());
		frameData.globalDescriptor.setBuffer(GLOBAL_DESC_LIGHT_BUFFER_BINDING, mGpuLd.getLightBuffer());
		frameData.globalDescriptor.finish(true);
	}

	
	mData->shader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::VERTEX, "assets/shader/default_rasterizer/shade.vert", { "GLSL" });
	mData->shader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::GEOMETRY, "assets/shader/default_rasterizer/shade.geom", { "GLSL" });
	mData->shader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::FRAGMENT, "assets/shader/default_rasterizer/shade.frag", { "GLSL" });
	mData->shader.finish();
	tamashii::FileWatcher::getInstance().watchFile("assets/shader/default_rasterizer/shade.vert", [this]() { mData->shader.reloadShader("assets/shader/default_rasterizer/shade.vert"); });
	tamashii::FileWatcher::getInstance().watchFile("assets/shader/default_rasterizer/shade.frag", [this]() { mData->shader.reloadShader("assets/shader/default_rasterizer/shade.frag"); });

	mData->pipelineCullNone.setShader(&mData->shader);
	mData->pipelineCullNone.addPushConstant(rvk::Shader::Stage::VERTEX | rvk::Shader::Stage::FRAGMENT, 0, 17 * sizeof(float));
	mData->pipelineCullNone.addDescriptorSet({ mGpuTd.getDescriptor(), &mData->frameData[0].globalDescriptor });

	mData->pipelineCullNone.addBindingDescription(0, sizeof(vertex_s), VK_VERTEX_INPUT_RATE_VERTEX);
	mData->pipelineCullNone.addAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_s, position));
	mData->pipelineCullNone.addAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_s, normal));
	mData->pipelineCullNone.addAttributeDescription(0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex_s, tangent));
	mData->pipelineCullNone.addAttributeDescription(0, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_s, texture_coordinates_0));
	mData->pipelineCullNone.addAttributeDescription(0, 4, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_s, texture_coordinates_1));
	mData->pipelineCullNone.addAttributeDescription(0, 5, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex_s, color_0));
	mData->pipelineCullNone.finish();
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	mData->pipelineCullFront = mData->pipelineCullNone;
	mData->pipelineCullFront.finish();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_BACK_BIT;
	mData->pipelineCullBack = mData->pipelineCullNone;
	mData->pipelineCullBack.finish();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
	mData->pipelineCullBoth = mData->pipelineCullNone;
	mData->pipelineCullBoth.finish();
	rvk::RPipeline::popRenderState();

	
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.polygonMode = VK_POLYGON_MODE_LINE;
	mData->pipelineWireframeCullNone = mData->pipelineCullNone;
	mData->pipelineWireframeCullNone.finish();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	mData->pipelineWireframeCullFront = mData->pipelineWireframeCullNone;
	mData->pipelineWireframeCullFront.finish();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_BACK_BIT;
	mData->pipelineWireframeCullBack = mData->pipelineWireframeCullNone;
	mData->pipelineWireframeCullBack.finish();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
	mData->pipelineWireframeCullBoth = mData->pipelineWireframeCullNone;
	mData->pipelineWireframeCullBoth.finish();
	rvk::RPipeline::popRenderState();

	prepareTBNVis();
	prepareBoundingBoxVis();
	

	RPipeline::global_render_state.renderpass = nullptr;
}
void DefaultRasterizer::destroy() {
	mGpuTd.destroy();
	mGpuMd.destroy();
	mGpuLd.destroy();
	tamashii::FileWatcher::getInstance().removeFile("assets/shader/default_rasterizer/shade.vert");
	tamashii::FileWatcher::getInstance().removeFile("assets/shader/default_rasterizer/shade.frag");

	mData.reset();
}

void DefaultRasterizer::prepareTBNVis() {
	
	for (uint32_t frameIndex = 0; frameIndex < mRoot.frameCount(); frameIndex++) {
		GpuData::GpuFrameData& frameData = mData->frameData[frameIndex];
		
		frameData.tbnVisDescriptor.addUniformBuffer(0, rvk::Shader::Stage::GEOMETRY);
		frameData.tbnVisDescriptor.setBuffer(0, &frameData.globalUniformBuffer);
		frameData.tbnVisDescriptor.finish(true);
	}
	mData->shaderTbnVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::VERTEX, "assets/shader/default_rasterizer/tbn_vis.vert", { "GLSL" });
	mData->shaderTbnVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::GEOMETRY, "assets/shader/default_rasterizer/tbn_vis.geom", { "GLSL" });
	mData->shaderTbnVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::FRAGMENT, "assets/shader/default_rasterizer/tbn_vis.frag", { "GLSL" });
	mData->shaderTbnVis.finish();

	mData->pipelineTbnVis.setShader(&mData->shaderTbnVis);
	mData->pipelineTbnVis.addPushConstant(rvk::Shader::Stage::VERTEX, 0, sizeof(glm::mat4) + sizeof(float));
	mData->pipelineTbnVis.addDescriptorSet({ &mData->frameData[0].tbnVisDescriptor });
	mData->pipelineTbnVis.addBindingDescription(0, sizeof(tamashii::vertex_s), VK_VERTEX_INPUT_RATE_VERTEX);
	mData->pipelineTbnVis.addAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(tamashii::vertex_s, position));
	mData->pipelineTbnVis.addAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(tamashii::vertex_s, normal));
	mData->pipelineTbnVis.addAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(tamashii::vertex_s, tangent));
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.dynamicStates.depth_bias = true;
	rvk::RPipeline::global_render_state.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	rvk::RPipeline::global_render_state.polygonMode = VK_POLYGON_MODE_FILL;
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_NONE;
	mData->pipelineTbnVis.finish();
	rvk::RPipeline::popRenderState();
}

void DefaultRasterizer::prepareBoundingBoxVis() {
	mData->shaderBoundingBoxVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::VERTEX, "assets/shader/default_rasterizer/bounding_box.vert", { "GLSL" });
	mData->shaderBoundingBoxVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::GEOMETRY, "assets/shader/default_rasterizer/bounding_box.geom", { "GLSL" });
	mData->shaderBoundingBoxVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::FRAGMENT, "assets/shader/default_rasterizer/bounding_box.frag", { "GLSL" });
	mData->shaderBoundingBoxVis.finish();

	mData->pipelineBoundingBoxVis.setShader(&mData->shaderBoundingBoxVis);
	mData->pipelineBoundingBoxVis.addPushConstant(rvk::Shader::Stage::VERTEX, 0, sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4));
	mData->pipelineBoundingBoxVis.addDescriptorSet({ &mData->frameData[0].tbnVisDescriptor });
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	rvk::RPipeline::global_render_state.polygonMode = VK_POLYGON_MODE_LINE;
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_NONE;
	mData->pipelineBoundingBoxVis.finish();
	rvk::RPipeline::popRenderState();
}

void DefaultRasterizer::prepareCircleVis() {
	mData->shaderCircleVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::VERTEX, "assets/shader/default_rasterizer/circle.vert", { "GLSL" });
	mData->shaderCircleVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::GEOMETRY, "assets/shader/default_rasterizer/circle.geom", { "GLSL" });
	mData->shaderCircleVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::FRAGMENT, "assets/shader/default_rasterizer/circle.frag", { "GLSL" });
	mData->shaderCircleVis.finish();

	mData->pipelineCircleVis.setShader(&mData->shaderCircleVis);
	mData->pipelineCircleVis.addPushConstant(rvk::Shader::Stage::VERTEX, 0, sizeof(glm::vec4));
	mData->pipelineCircleVis.addPushConstant(rvk::Shader::Stage::FRAGMENT, sizeof(glm::vec4), sizeof(glm::vec3));
	mData->pipelineCircleVis.addPushConstant(rvk::Shader::Stage::GEOMETRY, sizeof(glm::vec4) + sizeof(glm::vec3), sizeof(float));
	mData->pipelineCircleVis.addDescriptorSet({ &mData->frameData[0].tbnVisDescriptor });
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	rvk::RPipeline::global_render_state.polygonMode = VK_POLYGON_MODE_FILL;
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_NONE;
	mData->pipelineCircleVis.finish();
	rvk::RPipeline::popRenderState();
}


void DefaultRasterizer::sceneLoad(SceneBackendData aScene) {
	const GeometryDataVulkan::SceneInfo_s sinfo = GeometryDataVulkan::getSceneGeometryInfo(aScene);
	if (!sinfo.mMeshCount) mGpuGd.prepare();
	else mGpuGd.prepare(sinfo.mIndexCount, sinfo.mVertexCount);

	SingleTimeCommand stc = mRoot.singleTimeCommand();
	mGpuTd.loadScene(&stc, aScene);
	mGpuGd.loadScene(&stc, aScene);
	mGpuMd.loadScene(&stc, aScene, &mGpuTd);
	mGpuLd.loadScene(&stc, aScene, &mGpuTd);

	for (uint32_t idx = 0; idx < mRoot.frameCount(); idx++) {
		if (mGpuMd.bufferChanged(false)) mData->frameData[idx].globalDescriptor.setBuffer(GLOBAL_DESC_MATERIAL_BUFFER_BINDING, mGpuMd.getMaterialBuffer());
		mData->frameData[idx].globalDescriptor.update();
	}

	mShade = !aScene.refLights.empty();
}
void DefaultRasterizer::sceneUnload(SceneBackendData aScene) {
	mGpuTd.unloadScene();
	mGpuGd.unloadScene();
	mGpuMd.unloadScene();
	mGpuLd.unloadScene();

	mGpuGd.destroy();
}


void DefaultRasterizer::drawView(ViewDef_s* aViewDef) {
	CommandBuffer cb = mRoot.currentCmdBuffer();

	const glm::vec3 cc = glm::vec3(var::bg.value()[0], var::bg.value()[1], var::bg.value()[2]) / 255.0f;
	cb.cmdBeginRendering(
		{ { &mData->frameData[mRoot.currentIndex()].color,
		{{cc.x, cc.y, cc.z, 1.0f}} , RVK_LC, RVK_S2 } },
		{ &mData->frameData[mRoot.currentIndex()].depth,
		{0.0f, 0}, RVK_LC, RVK_S2 }
	);

	SingleTimeCommand stc = mRoot.singleTimeCommand();
	if (aViewDef->updates.mImages || aViewDef->updates.mTextures) {
		mRoot.device.waitIdle();
		mGpuTd.update(&stc, aViewDef->scene);
	}
	if (aViewDef->updates.mMaterials) mGpuMd.update(&stc, aViewDef->scene, &mGpuTd);
	if (aViewDef->updates.mLights) mGpuLd.update(&stc, aViewDef->scene, &mGpuTd);
	if (aViewDef->updates.mModelGeometries) mGpuGd.update(&stc, aViewDef->scene);

	
	GlobalUbo ubo {};
	ubo.viewMat = aViewDef->view_matrix;
	ubo.projMat = aViewDef->projection_matrix;
	ubo.inverseViewMat = aViewDef->inv_view_matrix;
	ubo.inverseProjMat = aViewDef->inv_projection_matrix;
	ubo.viewPos = glm::vec4{ aViewDef->view_pos, 1 };
	ubo.viewDir = glm::vec4{ aViewDef->view_dir, 0 };
	ubo.size[0] = static_cast<float>(aViewDef->target_size.x); ubo.size[1] = static_cast<float>(aViewDef->target_size.y);
	ubo.shade = mShade;
	ubo.dither_strength = mDitherStrength;
	ubo.light_count = mGpuLd.getLightCount();
	ubo.display_mode = mActiveDisplayMode;
	ubo.rgb_or_alpha = mRgbOrAlpha;
	ubo.normal_color = glm::vec4{ 0, 1, 0, 1 };
	ubo.tangent_color = glm::vec4{ 1, 0, 0, 1 };
	ubo.bitangent_color = glm::vec4{ 0, 0, 1, 1 };
	ubo.wireframe = mShowWireframe;
	ubo.useLightMaps = mUseLightMaps;
	ubo.mulVertexColors = mMulVertexColors;
	mData->frameData[mRoot.currentIndex()].globalUniformBuffer.STC_UploadData(&stc, &ubo, sizeof(GlobalUbo));
	Common::getInstance().intersectionSettings().mCullMode = static_cast<CullMode>(mActiveCullMode);

	
	if (mWireframe) {
		if (mActiveCullMode == 0) mData->currentPipeline = &mData->pipelineWireframeCullNone;
		if (mActiveCullMode == 1) mData->currentPipeline = &mData->pipelineWireframeCullFront;
		if (mActiveCullMode == 2) mData->currentPipeline = &mData->pipelineWireframeCullBack;
		if (mActiveCullMode == 3) mData->currentPipeline = &mData->pipelineWireframeCullBoth;
	} else {
		if (mActiveCullMode == 0) mData->currentPipeline = &mData->pipelineCullNone;
		if (mActiveCullMode == 1) mData->currentPipeline = &mData->pipelineCullFront;
		if (mActiveCullMode == 2) mData->currentPipeline = &mData->pipelineCullBack;
		if (mActiveCullMode == 3) mData->currentPipeline = &mData->pipelineCullBoth;
	}

	mData->currentPipeline->CMD_SetViewport(&cb);
	mData->currentPipeline->CMD_SetScissor(&cb);
	mData->currentPipeline->CMD_BindDescriptorSets(&cb, { mGpuTd.getDescriptor(), &mData->frameData[mRoot.currentIndex()].globalDescriptor });
	mData->currentPipeline->CMD_BindPipeline(&cb);
	mGpuGd.getVertexBuffer()->CMD_BindVertexBuffer(&cb, 0, 0);
	mGpuGd.getIndexBuffer()->CMD_BindIndexBuffer(&cb, VK_INDEX_TYPE_UINT32, 0);


	for (const DrawSurf_s& ds : aViewDef->surfaces) {
		int material_index = mGpuMd.getIndex(ds.refMesh->mesh->getMaterial());
		mData->currentPipeline->CMD_SetPushConstant(&cb, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16 * sizeof(float), &ds.model_matrix[0][0]);
		mData->currentPipeline->CMD_SetPushConstant(&cb, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 16 * sizeof(float), 1 * sizeof(int), &material_index);

		if (ds.refMesh->mesh->hasIndices()) mData->currentPipeline->CMD_DrawIndexed(&cb, static_cast<uint32_t>(ds.refMesh->mesh->getIndexCount()),
			mGpuGd.getOffset(ds.refMesh->mesh.get()).mIndexOffset, mGpuGd.getOffset(ds.refMesh->mesh.get()).mVertexOffset);
		else mData->currentPipeline->CMD_Draw(&cb, static_cast<uint32_t>(ds.refMesh->mesh->getVertexCount()), mGpuGd.getOffset(ds.refMesh->mesh.get()).mVertexOffset);
	}
	
	if (mShowTbn) drawTBNVis(&cb, aViewDef);
	if (mShowMeshBb || mShowModelBb) drawBoundingBoxVis(&cb, aViewDef);
	

	cb.cmdEndRendering();

	rvk::swapchain::CMD_BlitImageToImage(&cb, &mData->frameData[mRoot.currentIndex()].color, &mRoot.currentImage(), VK_FILTER_LINEAR);
}
void DefaultRasterizer::drawUI(UiConf_s* aUiConf) {
	if (ImGui::Begin("Settings", nullptr, 0))
	{
		ImGui::Separator();
		ImGui::Text("Cull Mode:");
		if (ImGui::BeginCombo("##cmcombo", mCullMode[mActiveCullMode].c_str(), ImGuiComboFlags_NoArrowButton)) {
			for (uint32_t i = 0; i < mCullMode.size(); i++) {
				const bool isSelected = (i == mActiveCullMode);
				if (ImGui::Selectable(mCullMode[i].c_str(), isSelected)) mActiveCullMode = i;
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::Text("Display Mode:");
		if (ImGui::BeginCombo("##dmcombo", mDisplayMode[mActiveDisplayMode].c_str(), ImGuiComboFlags_NoArrowButton)) {
			for (uint32_t i = 0; i < mDisplayMode.size(); i++) {
				const bool isSelected = (i == mActiveDisplayMode);
				if (ImGui::Selectable(mDisplayMode[i].c_str(), isSelected)) mActiveDisplayMode = i;
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (mActiveDisplayMode == 1)
		{
			ImGui::Checkbox("Show Alpha only", &mRgbOrAlpha);
		}

		if (mActiveDisplayMode == 0) {
			ImGui::Checkbox("Shade", &mShade);
			ImGui::Checkbox("Lines", &mWireframe);
			ImGui::Checkbox("Use Lightmaps", &mUseLightMaps);
		}
		ImGui::Checkbox("Multiply Vertex Color", &mMulVertexColors);
		if (mActiveDisplayMode == 0) {
			ImGui::DragFloat("##dither", &mDitherStrength, 0.01f, 0, 1.0f, "Dither: %.3f");
		}

		if (ImGui::CollapsingHeader("Debug", 0)) {
			ImGui::Checkbox("Wireframe Overlay", &mShowWireframe);
			ImGui::Checkbox("Show Model Bounding Box", &mShowModelBb);
			ImGui::Checkbox("Show Mesh Bounding Box", &mShowMeshBb);
			ImGui::Checkbox("Show TBN", &mShowTbn);
			if (mShowTbn) ImGui::DragFloat("##tbnscale", &mTbnScale, 0.001f, 0.001f, 0.2f, "Scale: %.3f");
			
			
		}
		ImGui::End();
	}
}

void DefaultRasterizer::drawTBNVis(const rvk::CommandBuffer* aCb, const ViewDef_s* aViewDef) {
	mData->pipelineTbnVis.CMD_SetDepthBias(aCb, mTbnDepthBiasConstant, mTbnDepthBiasSlope);
	mData->pipelineTbnVis.CMD_BindDescriptorSets(aCb, { &mData->frameData[mRoot.currentIndex()].tbnVisDescriptor });
	mData->pipelineTbnVis.CMD_BindPipeline(aCb);
	for (const DrawSurf_s& ds : aViewDef->surfaces) {
		mData->pipelineTbnVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &ds.model_matrix[0][0]);
		mData->pipelineTbnVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(float), &mTbnScale);
		mData->pipelineTbnVis.CMD_Draw(aCb, static_cast<uint32_t>(ds.refMesh->mesh->getVertexCount()), mGpuGd.getOffset(ds.refMesh->mesh.get()).mVertexOffset);
	}
}

void DefaultRasterizer::drawBoundingBoxVis(const rvk::CommandBuffer* aCb, const ViewDef_s* aViewDef) {
	mData->pipelineBoundingBoxVis.CMD_BindDescriptorSets(aCb, { &mData->frameData[mRoot.currentIndex()].tbnVisDescriptor });
	mData->pipelineBoundingBoxVis.CMD_BindPipeline(aCb);
	if (mShowMeshBb) {
		for (const DrawSurf_s& rm : aViewDef->surfaces) {
			const aabb_s aabb = rm.refMesh->mesh->getAABB();
			mData->pipelineBoundingBoxVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &rm.model_matrix[0][0]);
			mData->pipelineBoundingBoxVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec3), &aabb.mMin);
			mData->pipelineBoundingBoxVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::vec3), &aabb.mMax);
			mData->pipelineBoundingBoxVis.CMD_Draw(aCb, 1, 0);
		}
	}
	if (mShowModelBb) {
		for (const RefModel* rm : aViewDef->ref_models) {
			const aabb_s aabb = rm->model->getAABB();
			mData->pipelineBoundingBoxVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &rm->model_matrix[0][0]);
			mData->pipelineBoundingBoxVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec3), &aabb.mMin);
			mData->pipelineBoundingBoxVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(glm::vec3), &aabb.mMax);
			mData->pipelineBoundingBoxVis.CMD_Draw(aCb, 1, 0);
		}
	}
}

void DefaultRasterizer::drawCircleVis(const rvk::CommandBuffer* aCb, const ViewDef_s* aViewDef) {
	mData->pipelineCircleVis.CMD_BindDescriptorSets(aCb, { &mData->frameData[mRoot.currentIndex()].tbnVisDescriptor });
	mData->pipelineCircleVis.CMD_BindPipeline(aCb);

	for (const RefLight* l : aViewDef->lights) {
		float radius = 0.1f;
		glm::vec3 pos = l->position;
		glm::vec3 color = glm::vec3{ 220.0f / 255.0f, 220.0f / 255.0f, 220.0f / 255.0f };
		
		mData->pipelineCircleVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3), &pos[0]);
		mData->pipelineCircleVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec4), sizeof(glm::vec3), &color[0]);
		mData->pipelineCircleVis.CMD_SetPushConstant(aCb, VK_SHADER_STAGE_GEOMETRY_BIT, sizeof(glm::vec4) + sizeof(glm::vec3), sizeof(float), &radius);
		mData->pipelineCircleVis.CMD_Draw(aCb, 1, 0);
	}
		
}
