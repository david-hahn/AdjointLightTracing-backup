#include "ialt.hpp"
#include <imgui.h>
#include <glm/gtc/color_space.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/common/input.hpp>
#include <tamashii/core/scene/model.hpp>
#include "../../../assets/shader/ialt/defines.h"

T_USE_NAMESPACE
RVK_USE_NAMESPACE

constexpr uint32_t MAX_INDEX_DATA_SIZE = (10 * 1024 * 1024);
constexpr uint32_t MAX_VERTEX_DATA_SIZE = (8 * 512 * 1024);
constexpr uint32_t MAX_GLOBAL_IMAGE_SIZE = (1024);
constexpr uint32_t MAX_MATERIAL_SIZE = (2 * 1024);
constexpr uint32_t MAX_LIGHT_SIZE = (2 * 128);
constexpr uint32_t MAX_INSTANCE_SIZE = (4 * 1024);
constexpr uint32_t MAX_GEOMETRY_DATA_SIZE = (2 * 1024);

constexpr VkFormat COLOR_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

ccli::Var<int>		InteractiveAdjointLightTracing::mOptMaxIters("", "maxIters", 200, ccli::Flag::ConfigRead, "Limit optimizer iterations.");

void InteractiveAdjointLightTracing::windowSizeChanged(const uint32_t aWidth, const uint32_t aHeight)
{
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	
	mData->mDerivVisImageAccumulate.destroy();
	mData->mDerivVisImageAccumulateCount.destroy();
	
	mData->mDerivVisImageAccumulate.createImage2D(aWidth, aHeight, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	mData->mDerivVisImageAccumulate.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
	mData->mDerivVisImageAccumulateCount.createImage2D(aWidth, aHeight, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	mData->mDerivVisImageAccumulateCount.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);

	for (uint32_t si_idx = 0; si_idx < mRoot.frameCount(); si_idx++) {
		VkFrameData& frameData = mFrameData[si_idx];
		frameData.mColor.destroy();
		frameData.mDepth.destroy();

		frameData.mColor.createImage2D(aWidth, aHeight, COLOR_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::COLOR_ATTACHMENT);
		frameData.mDepth.createImage2D(aWidth, aHeight, DEPTH_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::DEPTH_STENCIL_ATTACHMENT);
		frameData.mColor.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		frameData.mDepth.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		frameData.mDerivVisImage.destroy();
		
		frameData.mDerivVisImage.createImage2D(aWidth, aHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		frameData.mDerivVisImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
		frameData.mDerivVisDescriptor.setImage(DERIV_VIS_DESC_DERIV_IMAGE_BINDING, &frameData.mDerivVisImage);
		frameData.mDerivVisDescriptor.setImage(DERIV_VIS_DESC_DERIV_ACC_IMAGE_BINDING, &mData->mDerivVisImageAccumulate);
		frameData.mDerivVisDescriptor.setImage(DERIV_VIS_DESC_DERIV_ACC_COUNT_IMAGE_BINDING, &mData->mDerivVisImageAccumulateCount);
		if(mSceneLoaded) frameData.mDerivVisDescriptor.update();
	}
	stc.end();
}

bool InteractiveAdjointLightTracing::drawOnMesh(const tamashii::DrawInfo* aDrawInfo)
{
	auto color = glm::vec4(0.0f);
	bool draw = false;
	bool left = false;



	if (InputSystem::getInstance().isDown(Input::MOUSE_LEFT)) {

		color = glm::vec4(glm::convertSRGBToLinear(glm::vec3(aDrawInfo->mColor0)), aDrawInfo->mColor0.w);
		draw = true;
		left = true;
	}



	else if (InputSystem::getInstance().isDown(Input::MOUSE_RIGHT)) {

		color = glm::vec4(glm::convertSRGBToLinear(glm::vec3(aDrawInfo->mColor1)), aDrawInfo->mColor1.w);
		draw = true;
	}
	if (draw) {
		const triangle_s tri = aDrawInfo->mHitInfo.mRefMeshHit->mesh->getTriangle(aDrawInfo->mHitInfo.mPrimitiveIndex);

		const auto refModel = std::static_pointer_cast<RefModel>(aDrawInfo->mHitInfo.mHit);
		const RefMesh* refMesh = aDrawInfo->mHitInfo.mRefMeshHit;
		refMesh->mesh->hasColors0(true);
		const GeometryDataVulkan::primitveBufferOffset_s offset = mData->mGpuBlas.getOffset(refMesh->mesh.get());
		const glm::vec4 hitPosWS = glm::vec4(aDrawInfo->mPositionWs, 1.0f);

		DrawBuffer db = {};
		db.modelMatrix = refModel->model_matrix;
		db.positionWS = aDrawInfo->mPositionWs;
		db.normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(refModel->model_matrix))) * tri.mGeoN);
		db.radius = aDrawInfo->mRadius;
		db.color = color;
		db.originWS = aDrawInfo->mHitInfo.mOriginPos;
		db.vertexOffset = offset.mVertexOffset;
		db.softBrush = aDrawInfo->mSoftBrush;
		db.drawAll = aDrawInfo->mDrawAll;
		db.drawRGB = aDrawInfo->mDrawRgb;
		db.drawALPHA = aDrawInfo->mDrawAlpha;
		db.leftMouse = left;
		db.setSH = mDrawSetSH;
		
		const CommandBuffer cb = mRoot.currentCmdBuffer();
		mData->mPipelineDrawOnMesh.CMD_SetPushConstant(&cb, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(db), &db);

		mData->mPipelineDrawOnMesh.CMD_BindDescriptorSets(&cb, { &mData->mDescriptorDrawOnMesh });
		mData->mPipelineDrawOnMesh.CMD_BindPipeline(&cb);

		mData->mPipelineDrawOnMesh.CMD_Dispatch(&cb, static_cast<uint32_t>(refMesh->mesh->getVertexCount()));
	}
	return false;
}


void InteractiveAdjointLightTracing::prepare(tamashii::RenderInfo_s& aRenderInfo) {
#ifdef IALT_USE_SPHERICAL_HARMONICS
	LightTraceOptimizer::sphericalHarmonicOrder = LightTraceOptimizer::vars::shOrder.value();
	LightTraceOptimizer::entries_per_vertex = 3 * (LightTraceOptimizer::sphericalHarmonicOrder + 1) * (LightTraceOptimizer::sphericalHarmonicOrder + 1); 
	spdlog::info("SH-order is {}", LightTraceOptimizer::sphericalHarmonicOrder);
	
#endif
	mData.emplace(mRoot.device);
	mFrameData.resize(mRoot.frameCount(), VkFrameData(mRoot.device));

	RPipeline::global_render_state.renderpass = nullptr;
	RPipeline::global_render_state.colorFormat = { COLOR_FORMAT };
	RPipeline::global_render_state.depthFormat = DEPTH_FORMAT;

	mData->mGpuTd.prepare(rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::ANY_HIT | rvk::Shader::Stage::COMPUTE | rvk::Shader::Stage::FRAGMENT, MAX_GLOBAL_IMAGE_SIZE);
	mData->mGpuMd.prepare(rvk::Buffer::Use::STORAGE, MAX_MATERIAL_SIZE);
	mData->mGpuLd.prepare(rvk::Buffer::Use::STORAGE, MAX_LIGHT_SIZE);
	constexpr uint32_t flags = rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::AS_INPUT;
	mData->mGpuBlas.prepare(MAX_INDEX_DATA_SIZE, MAX_VERTEX_DATA_SIZE, flags | rvk::Buffer::Use::INDEX, flags | rvk::Buffer::Use::VERTEX);
	for (auto& fd : mFrameData) fd.mGpuTlas.prepare(MAX_INSTANCE_SIZE, rvk::Buffer::Use::STORAGE, MAX_GEOMETRY_DATA_SIZE);

	mLto.init(&mData->mGpuTd, &mData->mGpuMd, &mData->mGpuLd, &mData->mGpuBlas, &mFrameData.front().mGpuTlas);

	mData->mGlobalBuffer.create(rvk::Buffer::Use::UNIFORM, sizeof(GlobalBufferR), rvk::Buffer::Location::HOST_COHERENT);
	mData->mGlobalBuffer.mapBuffer();

	mData->mDescriptor.reserve(4);
	mData->mDescriptor.addUniformBuffer(RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, rvk::Shader::Stage::VERTEX | rvk::Shader::Stage::GEOMETRY | rvk::Shader::Stage::FRAGMENT);
	mData->mDescriptor.addStorageBuffer(RASTERIZER_DESC_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::VERTEX | rvk::Shader::Stage::FRAGMENT);
	mData->mDescriptor.addStorageBuffer(RASTERIZER_DESC_TARGET_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::FRAGMENT);
	mData->mDescriptor.addStorageBuffer(RASTERIZER_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, rvk::Shader::Stage::FRAGMENT);
	mData->mDescriptor.addStorageBuffer(RASTERIZER_DESC_CHANNEL_WEIGHTS_BUFFER_BINDING, rvk::Shader::Stage::FRAGMENT);
	mData->mDescriptor.addStorageBuffer(RASTERIZER_DESC_VTX_COLOR_BUFFER_BINDING, rvk::Shader::Stage::VERTEX);
	mData->mDescriptor.addStorageBuffer(RASTERIZER_DESC_AREA_BUFFER_BINDING, rvk::Shader::Stage::FRAGMENT);
	mData->mDescriptor.addStorageBuffer(RASTERIZER_DESC_MATERIAL_BUFFER_BINDING, rvk::Shader::Stage::FRAGMENT);
	mData->mDescriptor.finish(false);

	uint32_t constData[3] = { LightTraceOptimizer::sphericalHarmonicOrder, LightTraceOptimizer::entries_per_vertex, (uint32_t)(LightTraceOptimizer::vars::unphysicalNicePreview.asBool().value()) };

	mData->mShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::VERTEX, IALT_SHADER_DIR "rasterizer_vertex.glsl", LightTraceOptimizer::shaderDefines);
	mData->mShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::GEOMETRY, IALT_SHADER_DIR "rasterizer_geometry.glsl", LightTraceOptimizer::shaderDefines);
	mData->mShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::FRAGMENT, IALT_SHADER_DIR "rasterizer_fragment.glsl", LightTraceOptimizer::shaderDefines);
	mData->mShader.addConstant(0, 0, 4u, 0u);
	mData->mShader.addConstant(0, 1, 4u, 4u);
	mData->mShader.addConstant(0, 2, 4u, 8u);
	mData->mShader.setConstantData(0, constData, 12u);
	mData->mShader.addConstant(2, 0, 4u, 0u);
	mData->mShader.addConstant(2, 1, 4u, 4u);
	mData->mShader.addConstant(2, 2, 4u, 8u);
	mData->mShader.setConstantData(2, constData, 12u);
	mData->mShader.finish();

	mData->mPipelineCullNone.setShader(&mData->mShader);
	mData->mPipelineCullNone.addPushConstant(rvk::Shader::Stage::VERTEX | rvk::Shader::Stage::FRAGMENT, 0, 17 * sizeof(float));
	mData->mPipelineCullNone.addDescriptorSet({ mData->mGpuTd.getDescriptor(), &mData->mDescriptor });

	mData->mPipelineCullNone.addBindingDescription(0, sizeof(vertex_s), VK_VERTEX_INPUT_RATE_VERTEX);
	mData->mPipelineCullNone.addAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_s, position));
	mData->mPipelineCullNone.addAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_s, normal));
	mData->mPipelineCullNone.addAttributeDescription(0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex_s, tangent));
	mData->mPipelineCullNone.addAttributeDescription(0, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_s, texture_coordinates_0));
	mData->mPipelineCullNone.addAttributeDescription(0, 4, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_s, texture_coordinates_1));
	mData->mPipelineCullNone.addAttributeDescription(0, 5, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex_s, color_0));

	mData->mPipelineCullNone.finish();
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	mData->mPipelineCullFront = mData->mPipelineCullNone;
	mData->mPipelineCullFront.finish();
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_BACK_BIT;
	mData->mPipelineCullBack = mData->mPipelineCullNone;
	mData->mPipelineCullBack.finish();
	rvk::RPipeline::popRenderState();

#ifdef IALT_USE_SPHERICAL_HARMONICS
	mData->mShaderSHVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::VERTEX, IALT_SHADER_DIR "sh_vis.vert", LightTraceOptimizer::shaderDefines);
	mData->mShaderSHVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::GEOMETRY, IALT_SHADER_DIR "sh_vis.geom", LightTraceOptimizer::shaderDefines);
	mData->mShaderSHVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::FRAGMENT, IALT_SHADER_DIR "sh_vis.frag", LightTraceOptimizer::shaderDefines);
	mData->mShaderSHVis.addConstant(0, 0, 4u, 0u);
	mData->mShaderSHVis.addConstant(2, 0, 4u, 0u);
	mData->mShaderSHVis.addConstant(2, 1, 4u, 4u);
	mData->mShaderSHVis.setConstantData(0, &LightTraceOptimizer::entries_per_vertex, 4u);
	mData->mShaderSHVis.setConstantData(2, &constData[0], 8u);
	mData->mShaderSHVis.finish();

	mData->mPipelineSHVisNone.setShader(&mData->mShaderSHVis);
	mData->mPipelineSHVisNone.addPushConstant(rvk::Shader::Stage::VERTEX, 0, sizeof(glm::mat4));
	mData->mPipelineSHVisNone.addPushConstant(rvk::Shader::Stage::GEOMETRY | rvk::Shader::Stage::FRAGMENT, sizeof(glm::mat4), sizeof(float));

	mData->mPipelineSHVisNone.addBindingDescription(0, sizeof(vertex_s), VK_VERTEX_INPUT_RATE_VERTEX);
	mData->mPipelineSHVisNone.addAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_s, position));
	mData->mPipelineSHVisNone.addAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_s, normal));
	mData->mPipelineSHVisNone.addAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_s, tangent));
	mData->mPipelineSHVisNone.addDescriptorSet({ &mData->mDescriptor });
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	rvk::RPipeline::global_render_state.polygonMode = VK_POLYGON_MODE_FILL;
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_NONE;
	mData->mPipelineSHVisNone.finish();
	rvk::RPipeline::popRenderState();

	
	mData->mShaderSingleSHVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::VERTEX, IALT_SHADER_DIR "single_sh_vis.vert", LightTraceOptimizer::shaderDefines);
	mData->mShaderSingleSHVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::GEOMETRY, IALT_SHADER_DIR "single_sh_vis.geom", LightTraceOptimizer::shaderDefines);
	mData->mShaderSingleSHVis.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::FRAGMENT, IALT_SHADER_DIR "single_sh_vis.frag", LightTraceOptimizer::shaderDefines);
	mData->mShaderSingleSHVis.addConstant(2, 0, 4u, 0u);
	mData->mShaderSingleSHVis.addConstant(2, 1, 4u, 4u);
	mData->mShaderSingleSHVis.setConstantData(2, &constData[0], 8u);
	mData->mShaderSingleSHVis.finish();

	mData->mPipelineSingleSHVisNone.setShader(&mData->mShaderSingleSHVis);
	mData->mPipelineSingleSHVisNone.addPushConstant(rvk::Shader::Stage::VERTEX, 0u, 16u);
	mData->mPipelineSingleSHVisNone.addPushConstant(rvk::Shader::Stage::FRAGMENT, 16u, 16u + 8u + 4u);
	mData->mPipelineSingleSHVisNone.addPushConstant(rvk::Shader::Stage::GEOMETRY, 16u * 2u + 8u, 4u);

	mData->mPipelineSingleSHVisNone.addDescriptorSet({ &mData->mDescriptor });
	rvk::RPipeline::pushRenderState();
	rvk::RPipeline::global_render_state.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	rvk::RPipeline::global_render_state.polygonMode = VK_POLYGON_MODE_FILL;
	rvk::RPipeline::global_render_state.cullMode = VK_CULL_MODE_NONE;
	mData->mPipelineSingleSHVisNone.finish();
	rvk::RPipeline::popRenderState();
#endif

	mData->mDescriptorDrawOnMesh.reserve(2);
	mData->mDescriptorDrawOnMesh.addStorageBuffer(DRAW_DESC_VERTEX_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mData->mDescriptorDrawOnMesh.addStorageBuffer(DRAW_DESC_TARGET_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mData->mDescriptorDrawOnMesh.addStorageBuffer(DRAW_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mData->mDescriptorDrawOnMesh.finish(false);

	mData->mShaderDrawOnMesh.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::COMPUTE, IALT_SHADER_DIR "draw_on_mesh.comp", LightTraceOptimizer::shaderDefines);
	mData->mShaderDrawOnMesh.addConstant(0, 0, 4u, 0u);
	mData->mShaderDrawOnMesh.addConstant(0, 1, 4u, 4u);
	mData->mShaderDrawOnMesh.setConstantData(0, &constData[0], 8u);
	mData->mShaderDrawOnMesh.finish();

	mData->mPipelineDrawOnMesh.setShader(&mData->mShaderDrawOnMesh);
	mData->mPipelineDrawOnMesh.addDescriptorSet({ &mData->mDescriptorDrawOnMesh });
	mData->mPipelineDrawOnMesh.addPushConstant(rvk::Shader::Stage::COMPUTE, 0u, sizeof(DrawBuffer));
	mData->mPipelineDrawOnMesh.finish();

	mData->mDerivVisShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, IALT_SHADER_DIR "deriv_vis_rgen.glsl", LightTraceOptimizer::shaderDefines);		
	mData->mDerivVisShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::CLOSEST_HIT, IALT_SHADER_DIR "forward_rchit.glsl", LightTraceOptimizer::shaderDefines);	
	mData->mDerivVisShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::MISS, IALT_SHADER_DIR "forward_rmiss.glsl", LightTraceOptimizer::shaderDefines);			
	mData->mDerivVisShader.addGeneralShaderGroup(0);
	mData->mDerivVisShader.addHitShaderGroup(1);
	mData->mDerivVisShader.addGeneralShaderGroup(2);
	mData->mDerivVisShader.addConstant(0, 0, 4u, 0u);
	mData->mDerivVisShader.addConstant(0, 1, 4u, 4u);
	mData->mDerivVisShader.setConstantData(0, &constData[0], 8u);
	mData->mDerivVisShader.finish();

	SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	
	for (uint32_t frameIndex = 0; frameIndex < mRoot.frameCount(); frameIndex++) {
		VkFrameData& frameData = mFrameData[frameIndex];
		frameData.mColor.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, COLOR_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::COLOR_ATTACHMENT);
		frameData.mDepth.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, DEPTH_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::DEPTH_STENCIL_ATTACHMENT);
		frameData.mColor.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		frameData.mDepth.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	mData->mDerivVisImageAccumulate.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, VK_FORMAT_R32G32B32A32_SFLOAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
	mData->mDerivVisImageAccumulate.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
	mData->mDerivVisImageAccumulateCount.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, VK_FORMAT_R32_UINT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
	mData->mDerivVisImageAccumulateCount.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
	for (uint32_t idx = 0; idx < mRoot.frameCount(); idx++) {
		VkFrameData& frameData = mFrameData[idx];
		frameData.mDerivVisImage.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, VK_FORMAT_B8G8R8A8_UNORM, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
		frameData.mDerivVisImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);

		frameData.mDerivVisDescriptor.reserve(3);
		frameData.mDerivVisDescriptor.addStorageImage(DERIV_VIS_DESC_DERIV_IMAGE_BINDING, rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::COMPUTE);
		frameData.mDerivVisDescriptor.addStorageImage(DERIV_VIS_DESC_DERIV_ACC_IMAGE_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.mDerivVisDescriptor.addStorageImage(DERIV_VIS_DESC_DERIV_ACC_COUNT_IMAGE_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.mDerivVisDescriptor.addUniformBuffer(DERIV_VIS_DESC_UBO_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.mDerivVisDescriptor.addStorageBuffer(DERIV_VIS_DESC_TARGET_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.mDerivVisDescriptor.addStorageBuffer(DERIV_VIS_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.mDerivVisDescriptor.addStorageBuffer(DERIV_VIS_DESC_FD_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.mDerivVisDescriptor.addStorageBuffer(DERIV_VIS_DESC_FD2_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);

		frameData.mDerivVisDescriptor.setImage(DERIV_VIS_DESC_DERIV_IMAGE_BINDING, &frameData.mDerivVisImage);
		frameData.mDerivVisDescriptor.setImage(DERIV_VIS_DESC_DERIV_ACC_IMAGE_BINDING, &mData->mDerivVisImageAccumulate);
		frameData.mDerivVisDescriptor.setImage(DERIV_VIS_DESC_DERIV_ACC_COUNT_IMAGE_BINDING, &mData->mDerivVisImageAccumulateCount);
		frameData.mDerivVisDescriptor.setBuffer(DERIV_VIS_DESC_UBO_BUFFER_BINDING, &mData->mGlobalBuffer);
		frameData.mDerivVisDescriptor.finish(false);
	}
	stc.end();

	mData->mDerivVisPipeline.setShader(&mData->mDerivVisShader);
	mData->mDerivVisPipeline.addDescriptorSet({ mData->mGpuTd.getDescriptor(), mLto.getAdjointDescriptor(), &mFrameData[0].mDerivVisDescriptor });
	mData->mDerivVisPipeline.finish();
}
void InteractiveAdjointLightTracing::destroy() {
	mData.reset();
	mFrameData.clear();

	mLto.destroy();
}


void InteractiveAdjointLightTracing::sceneLoad(tamashii::SceneBackendData aScene) {
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	uint64_t vertexCount = 0;

	
	
	
	mLights = nullptr;
	mLto.getLightOptParams().clear();

	
	for (auto model : aScene.models) for (const auto& mesh : *model) vertexCount += mesh->getVertexCount();
	if (!vertexCount) return;

	mLights = &aScene.refLights;

	mData->mGpuTd.loadScene(&stc, aScene);
	mData->mGpuMd.loadScene(&stc, aScene, &mData->mGpuTd);
	mData->mGpuBlas.loadScene(&stc, aScene);
	mData->mGpuLd.loadScene(&stc, aScene, &mData->mGpuTd, &mData->mGpuBlas);
	for (auto& fd : mFrameData) fd.mGpuTlas.loadScene(&stc, aScene, &mData->mGpuBlas, &mData->mGpuMd);

	mData->mRadianceBufferCopy.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD, vertexCount * LightTraceOptimizer::entries_per_vertex * sizeof(float), rvk::Buffer::Location::DEVICE);
	mData->mFdRadianceBufferCopy.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD, vertexCount * LightTraceOptimizer::entries_per_vertex * sizeof(float), rvk::Buffer::Location::DEVICE);
	mData->mFd2RadianceBufferCopy.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD, vertexCount * LightTraceOptimizer::entries_per_vertex * sizeof(float), rvk::Buffer::Location::DEVICE);

	stc.begin();
	mData->mRadianceBufferCopy.CMD_FillBuffer(stc.buffer(), 0);
	mData->mFdRadianceBufferCopy.CMD_FillBuffer(stc.buffer(), 0);
	mData->mFd2RadianceBufferCopy.CMD_FillBuffer(stc.buffer(), 0);

	stc.begin();
	mData->mRadianceBufferCopy.CMD_FillBuffer(stc.buffer(), 0);
	stc.end();
	mLto.sceneLoad(aScene, vertexCount);
	if (mData->mGpuLd.getLightCount()) {
		mLto.forward(mLto.getCurrentParams(), &mData->mRadianceBufferCopy);
	}

	mData->mDescriptor.setBuffer(RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, &mData->mGlobalBuffer);
	mData->mDescriptor.setBuffer(RASTERIZER_DESC_RADIANCE_BUFFER_BINDING, &mData->mRadianceBufferCopy);
	mData->mDescriptor.setBuffer(RASTERIZER_DESC_VTX_COLOR_BUFFER_BINDING, mLto.getVtxTextureColorBuffer());
	mData->mDescriptor.setBuffer(RASTERIZER_DESC_AREA_BUFFER_BINDING, mLto.getVtxAreaBuffer());
	mData->mDescriptor.setBuffer(RASTERIZER_DESC_MATERIAL_BUFFER_BINDING, mData->mGpuMd.getMaterialBuffer());
	mData->mDescriptor.setBuffer(RASTERIZER_DESC_TARGET_RADIANCE_BUFFER_BINDING, mLto.getTargetRadianceBuffer());
	mData->mDescriptor.setBuffer(RASTERIZER_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, mLto.getTargetRadianceWeightsBuffer());
	mData->mDescriptor.setBuffer(RASTERIZER_DESC_CHANNEL_WEIGHTS_BUFFER_BINDING, mLto.getChannelWeightsBuffer());
	mData->mDescriptor.update();

	mData->mDescriptorDrawOnMesh.setBuffer(DRAW_DESC_VERTEX_BUFFER_BINDING, mData->mGpuBlas.getVertexBuffer());
	mData->mDescriptorDrawOnMesh.setBuffer(DRAW_DESC_TARGET_RADIANCE_BUFFER_BINDING, mLto.getTargetRadianceBuffer());
	mData->mDescriptorDrawOnMesh.setBuffer(DRAW_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, mLto.getTargetRadianceWeightsBuffer());
	mData->mDescriptorDrawOnMesh.update();

	for (uint32_t idx = 0; idx < mRoot.frameCount(); idx++) {
		VkFrameData& frameData = mFrameData[idx];
		frameData.mDerivVisDescriptor.setBuffer(DERIV_VIS_DESC_TARGET_BUFFER_BINDING, mLto.getTargetRadianceBuffer());
		frameData.mDerivVisDescriptor.setBuffer(DERIV_VIS_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, mLto.getTargetRadianceWeightsBuffer());
		frameData.mDerivVisDescriptor.setBuffer(DERIV_VIS_DESC_FD_RADIANCE_BUFFER_BINDING, &mData->mFdRadianceBufferCopy);
		frameData.mDerivVisDescriptor.setBuffer(DERIV_VIS_DESC_FD2_RADIANCE_BUFFER_BINDING, &mData->mFd2RadianceBufferCopy);
		frameData.mDerivVisDescriptor.update();
	}

	if (mFDGradImageSelection.has_value()) mLto.fillFiniteDiffRadianceBuffer(mFDGradImageSelection.value().first, mFDGradImageSelection.value().second, mFdGradH, &mData->mFdRadianceBufferCopy, &mData->mFd2RadianceBufferCopy);
	mSceneLoaded = true;

	if (!LightTraceOptimizer::vars::runPredefinedTest.value().empty()) {
		spdlog::info("running test case {}", LightTraceOptimizer::vars::runPredefinedTest.value());
		auto testname = LightTraceOptimizer::vars::runPredefinedTest.value();

		startOptimizerThread([aScene, testname](InteractiveAdjointLightTracing* ialt, LightTraceOptimizer* aLto, rvk::Buffer* aRadianceBufferOut, unsigned int, float, int) {
			aLto->runPredefinedTestCase(ialt, aScene, testname, aRadianceBufferOut);
		});
	}
}

void InteractiveAdjointLightTracing::sceneUnload(tamashii::SceneBackendData aScene) {
	mSceneLoaded = false;
	mData->mGpuTd.unloadScene();
	mData->mGpuMd.unloadScene();
	mData->mGpuLd.unloadScene();
	mData->mGpuBlas.unloadScene();
	for (auto& fd : mFrameData) fd.mGpuTlas.unloadScene();


	mLto.sceneUnload(aScene);
	mData->mRadianceBufferCopy.destroy();
	mData->mFdRadianceBufferCopy.destroy();
	mData->mFd2RadianceBufferCopy.destroy();
}


void InteractiveAdjointLightTracing::drawView(tamashii::ViewDef_s* aViewDef) {
	mNextFrameCV.notify_all();
	
	CommandBuffer& cb = mRoot.currentCmdBuffer();
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	const auto cc = glm::vec3{var::varToVec(var::bg)} / 255.0f;
	cb.cmdBeginRendering(
		{ { &mFrameData[mRoot.currentIndex()].mColor,
		{{cc.x, cc.y, cc.z, 1.0f}} , RVK_LC, RVK_S2 } },
		{ &mFrameData[mRoot.currentIndex()].mDepth,
		{0.0f, 0}, RVK_LC, RVK_S2 }
	);

	GlobalBufferR buffer = {};
	buffer.viewMat = aViewDef->view_matrix;
	buffer.projMat = aViewDef->projection_matrix;
	buffer.inverseViewMat = aViewDef->inv_view_matrix;
	buffer.inverseProjMat = aViewDef->inv_projection_matrix;
	buffer.viewPos = glm::vec4(aViewDef->view_pos, 1);
	buffer.viewDir = glm::vec4(aViewDef->view_dir, 0);
	buffer.wireframeColor[0] = mWireframeColor.x; buffer.wireframeColor[1] = mWireframeColor.y; buffer.wireframeColor[2] = mWireframeColor.z; buffer.wireframeColor[3] = 1.0f;
	buffer.size[0] = static_cast<float>(aViewDef->target_size.x); buffer.size[1] = static_cast<float>(aViewDef->target_size.y);
	buffer.shade = 0;
	if (!LightTraceOptimizer::vars::constRandSeed) buffer.frameCount = aViewDef->frame_index;
	buffer.grad_vis_accumulate = mGradVisAcc;
	buffer.show_target = mShowTarget;
	buffer.show_alpha = mShowAlpha;
	buffer.show_adjoint_deriv = mShowAdjointDeriv;
	buffer.show_wireframe_overlay = mShowWireframeOverlay;
	buffer.adjoint_range = mAdjointVisRange;
	buffer.log_adjoint_vis = mAdjointVisLog;
	buffer.dither_strength = 0;
	buffer.grad_range = mGradRange;
	buffer.bg_color = glm::vec4(cc, 1.0f);
	buffer.cull_mode = mActiveCullMode;
	buffer.log_grad_vis = mGradVisLog;
	buffer.grad_vis_weights = mGradVisWeights;
	buffer.fd_grad_vis = mShowFDGrad;
	buffer.fd_grad_h = mFdGradH;
	if (mGradImageSelection.has_value())
	{
		buffer.light_deriv_idx = mGradImageSelection.value().first->ref_light_index;
		buffer.param_deriv_idx = mGradImageSelection.value().second;
	}
	else if (mFDGradImageSelection.has_value())
	{
		buffer.light_deriv_idx = mFDGradImageSelection.value().first->ref_light_index;
		buffer.param_deriv_idx = mFDGradImageSelection.value().second;
	}
	mData->mGlobalBuffer.STC_UploadData(&stc, &buffer, sizeof(GlobalBufferR));

	if (aViewDef->updates.mLights) {
		if (aViewDef->updates.mImages || aViewDef->updates.mTextures) {
			mRoot.device.waitIdle();
			mData->mGpuTd.update(&stc, aViewDef->scene);
		}
		mData->mGpuLd.update(&stc, aViewDef->scene, &mData->mGpuTd, &mData->mGpuBlas);
		mLto.updateParamsFromScene();
		if (mFDGradImageSelection.has_value()) mLto.fillFiniteDiffRadianceBuffer(mFDGradImageSelection.value().first, mFDGradImageSelection.value().second, mFdGradH, &mData->mFdRadianceBufferCopy, &mData->mFd2RadianceBufferCopy);
		else mLto.forward(mLto.getCurrentParams(), &mData->mRadianceBufferCopy);
	}
	if (aViewDef->updates.mModelGeometries) {
		mData->mGpuBlas.update(&stc, aViewDef->scene);

		
		
	}

	Common::getInstance().intersectionSettings().mCullMode = static_cast<tamashii::CullMode>(mActiveCullMode);
	if (mActiveCullMode == 0) mData->mCurrentPipeline = &mData->mPipelineCullNone;
	else if (mActiveCullMode == 1) mData->mCurrentPipeline = &mData->mPipelineCullFront;
	else if (mActiveCullMode == 2) mData->mCurrentPipeline = &mData->mPipelineCullBack;

	if (!aViewDef->surfaces.empty()) {
		mData->mCurrentPipeline->CMD_BindDescriptorSets(&cb, { mData->mGpuTd.getDescriptor(), &mData->mDescriptor });
		mData->mCurrentPipeline->CMD_BindPipeline(&cb);
		mData->mGpuBlas.getVertexBuffer()->CMD_BindVertexBuffer(&cb, 0, 0);
		mData->mGpuBlas.getIndexBuffer()->CMD_BindIndexBuffer(&cb, VK_INDEX_TYPE_UINT32, 0);
	}

	mData->mCurrentPipeline->CMD_SetViewport(&cb);
	mData->mCurrentPipeline->CMD_SetScissor(&cb);
	for (const DrawSurf_s& ds : aViewDef->surfaces) {
		
		if (!ds.refMesh->mesh->getMaterial()->getCullBackface()) mData->mPipelineCullNone.CMD_BindPipeline(&cb);
		else mData->mCurrentPipeline->CMD_BindPipeline(&cb);

		int materialIndex = mData->mGpuMd.getIndex(ds.refMesh->mesh->getMaterial());
		mData->mCurrentPipeline->CMD_SetPushConstant(&cb, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16 * sizeof(float), &ds.model_matrix[0][0]);
		mData->mCurrentPipeline->CMD_SetPushConstant(&cb, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 16 * sizeof(float), 1 * sizeof(int), &materialIndex);

		if (ds.refMesh->mesh->hasIndices()) mData->mCurrentPipeline->CMD_DrawIndexed(&cb, static_cast<uint32_t>(ds.refMesh->mesh->getIndexCount()),
			mData->mGpuBlas.getOffset(ds.refMesh->mesh.get()).mIndexOffset, mData->mGpuBlas.getOffset(ds.refMesh->mesh.get()).mVertexOffset);
		else mData->mCurrentPipeline->CMD_Draw(&cb, static_cast<uint32_t>(ds.refMesh->mesh->getVertexCount()), mData->mGpuBlas.getOffset(ds.refMesh->mesh.get()).mVertexOffset);
	}

#ifdef IALT_USE_SPHERICAL_HARMONICS
	if (mShowSH) {
		/*mSwapchain->getCurrentDepthImage()->CMD_ImageBarrier(cb,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);*/
		mData->mPipelineSHVisNone.CMD_BindPipeline(&cb);
		mData->mPipelineSHVisNone.CMD_BindDescriptorSets(&cb, { &mData->mDescriptor });
		for (const DrawSurf_s& ds : aViewDef->surfaces) {
			mData->mPipelineSHVisNone.CMD_SetPushConstant(&cb, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &ds.model_matrix[0][0]);
			mData->mPipelineSHVisNone.CMD_SetPushConstant(&cb, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 16 * sizeof(float), 1 * sizeof(float), &mShowSHSize);
			mData->mPipelineSHVisNone.CMD_Draw(&cb, static_cast<uint32_t>(ds.refMesh->mesh->getVertexCount()), mData->mGpuBlas.getOffset(ds.refMesh->mesh.get()).mVertexOffset);
		}
	}

	if (mShowInterpolatedSH) {
		const IntersectionSettings settings(CullMode::None, HitMask::Geometry);
		Intersection info = {};
		Common::getInstance().intersectScene(settings, &info);
		if (info.mHit && info.mHit->type == Ref::Type::Model) {
			glm::uvec4 offsets = { 0,0,0,0 };
			bool stop = false;
			for (auto model : aViewDef->scene.refModels) {
				for (const auto& mesh : model->refMeshes) {
					if (mesh.get() != info.mRefMeshHit) offsets += mesh->mesh->getVertexCount();
					else {
						offsets.x += mesh->mesh->getIndicesArray()[3u * info.mPrimitiveIndex + 0];
						offsets.y += mesh->mesh->getIndicesArray()[3u * info.mPrimitiveIndex + 1];
						offsets.z += mesh->mesh->getIndicesArray()[3u * info.mPrimitiveIndex + 2];
						stop = true;
					}
					if (stop) break;
				}
				if (stop) break;
			}

			/*mSwapchain->getCurrentDepthImage()->CMD_ImageBarrier(cb,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);*/
			mData->mPipelineSingleSHVisNone.CMD_BindPipeline(&cb);
			mData->mPipelineSingleSHVisNone.CMD_BindDescriptorSets(&cb, { &mData->mDescriptor });

			mData->mPipelineSingleSHVisNone.CMD_SetPushConstant(&cb, VK_SHADER_STAGE_VERTEX_BIT, 0, 16u, &info.mHitPos);
			mData->mPipelineSingleSHVisNone.CMD_SetPushConstant(&cb, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 16u * 2u + 8u, 4u, &mShowSHSize);
			mData->mPipelineSingleSHVisNone.CMD_SetPushConstant(&cb, VK_SHADER_STAGE_FRAGMENT_BIT, 16u, 16u, &offsets);
			mData->mPipelineSingleSHVisNone.CMD_SetPushConstant(&cb, VK_SHADER_STAGE_FRAGMENT_BIT, 16u * 2u, 8u, &info.mBarycentric);
			mData->mPipelineSingleSHVisNone.CMD_Draw(&cb, 1);
		}
	}
#endif
	cb.cmdEndRendering();
	rvk::swapchain::CMD_BlitImageToImage(&cb, &mFrameData[mRoot.currentIndex()].mColor, &mRoot.currentImage(), VK_FILTER_LINEAR);

	if (!mShowGrad) mGradImageSelection.reset();
	if (!mShowFDGrad) mFDGradImageSelection.reset();
	if (mGradImageSelection.has_value() || mFDGradImageSelection.has_value()) {
		if (!aViewDef->scene.refModels.empty()) {
			mData->mDerivVisPipeline.CMD_BindDescriptorSets(&cb, { mData->mGpuTd.getDescriptor(), mLto.getAdjointDescriptor(), &mFrameData[mRoot.currentIndex()].mDerivVisDescriptor });
			mData->mDerivVisPipeline.CMD_BindPipeline(&cb);
			mData->mDerivVisPipeline.CMD_TraceRays(&cb, aViewDef->target_size.x, aViewDef->target_size.y, 1);
		}
		rvk::swapchain::CMD_BlitImageToImage(&cb, &mFrameData[mRoot.currentIndex()].mDerivVisImage, &mRoot.currentImage(), VK_FILTER_LINEAR);
	}

}
void InteractiveAdjointLightTracing::drawUI(tamashii::UiConf_s* aUiConf) {
	if (ImGui::Begin("Settings", nullptr, 0))
	{
		bool fwdPT = mLto.useForwardPT();
		bool bwdPT = mLto.useBackwardPT();
		ImGui::Separator();
		ImGui::PushItemWidth(110);
		if (!fwdPT || !bwdPT) {
			int xRays = LightTraceOptimizer::vars::numRaysXperLight.value();
			int yRays = LightTraceOptimizer::vars::numRaysYperLight.value();
			if (ImGui::SliderInt("##xRay", &xRays, 1, 10000, "xRays: %d")) LightTraceOptimizer::vars::numRaysXperLight.value(xRays);
			ImGui::SameLine();
			if (ImGui::SliderInt("##yRay", &yRays, 0, 10000, "yRays: %d")) LightTraceOptimizer::vars::numRaysYperLight.value(yRays);
		}
		if (fwdPT || bwdPT) {
			int triRays = LightTraceOptimizer::vars::numRaysPerTriangle.value();
			
			if (ImGui::SliderInt("##triRays", &triRays, 1, 10000, "triRays: %d")) LightTraceOptimizer::vars::numRaysPerTriangle.value(triRays);
			
			
		}
		ImGui::PopItemWidth();
		ImGui::PushItemWidth(140);
		if (ImGui::BeginCombo("##opti_combo", mOptimizerChoices[mOptimizerChoice].c_str())) {
			for (size_t i = 0; i < mOptimizerChoices.size(); i++) {
				const bool isSelected = (i == mOptimizerChoice);
				if (ImGui::Selectable(mOptimizerChoices[i].c_str(), isSelected)) mOptimizerChoice = i;
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		if (mLto.optimizationRunning()) {
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.3f, 0.4f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.6f, 0.3f, 0.4f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.7f, 0.4f, 0.5f, 1.0f });

			if (ImGui::Button("Stop", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
			{
				mLto.optimizationRunning(false);
			}
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.4f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.3f, 0.6f, 0.4f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.4f, 0.7f, 0.5f, 1.0f });
            if (ImGui::Button("Optimize", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
				auto scene = aUiConf->scene;
				startOptimizerThread([scene](InteractiveAdjointLightTracing* ialt, LightTraceOptimizer* aLto, rvk::Buffer* aRadianceBufferOut, unsigned int aOpti, float aOptimizerStepSize, int aOptMaxIters) {
					aLto->optimize(aOpti, aRadianceBufferOut, aOptimizerStepSize, aOptMaxIters);
					scene->requestLightUpdate();
				});
            }
		}
		ImGui::PopStyleColor(3);
		int tmpInt = mOptMaxIters.value();
		if (ImGui::SliderInt("##maxIters", &tmpInt, 1, 5000, "max iters: %d")) mOptMaxIters.value(tmpInt);

		if (mLto.getCurrentHistoryIndex() != -1) {
			if (ImGui::SliderInt("##H", &mLto.getCurrentHistoryIndex(), 0, static_cast<int>(mLto.getHistorySize()) - 1, "History: %d"))
			{
				mLto.selectParamsFromHistory(mLto.getCurrentHistoryIndex());
				aUiConf->scene->requestLightUpdate();
			}
		}
		if (ImGui::Button("Copy Radiance to Target", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
		{
			mLto.copyRadianceToTarget();
			mLto.buildObjectiveFunction(Common::getInstance().getRenderSystem()->getMainScene().get()->getSceneData());
		}
		if (ImGui::Button("Copy Target to Mesh", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
		{
			mLto.copyTargetToMesh(Common::getInstance().getRenderSystem()->getMainScene().get()->getSceneData());
		}
		static float weight = 1.0f;
		ImGui::PushItemWidth(110);
		ImGui::DragFloat("##weight", &weight, 0.01f, 0.0, 1.0, "Weight: %.3g");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Set Weights", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
		{
			mLto.setTargetWeights(weight);
		}
		ImGui::DragFloat("##IS", &mOptimizerStepSize, 0.01f, 0.01f, 5.0f, "Optim step size: %.3g");
		if(ImGui::DragInt("##IB", &mLto.bounces(), 1, 0, 10, "Bounces: %d")) aUiConf->scene->requestLightUpdate();

		ImGui::Separator();
		ImGui::Checkbox("Show Wireframe Overlay", &mShowWireframeOverlay);
		if(mShowWireframeOverlay) ImGui::ColorEdit3("Wireframe Color", &mWireframeColor[0], ImGuiColorEditFlags_NoInputs);
		if (!mShowAdjointDeriv && !mShowGrad) ImGui::Checkbox("Show Target", &mShowTarget);
		if (mShowTarget) ImGui::Checkbox("Show Weights", &mShowAlpha);
		if (!mShowTarget && !mShowGrad) ImGui::Checkbox("Show Adjoint", &mShowAdjointDeriv);
		if (mShowAdjointDeriv)
		{
			ImGui::DragFloat("##adjrange", &mAdjointVisRange, 0.01f, 0.001f, 100.0f, "Adjoint: %f");
			ImGui::Checkbox("Log", &mAdjointVisLog);
		}
		if (!mShowTarget && !mShowAdjointDeriv && mShowGrad) {
			ImGui::Checkbox("Show Gradient", &mShowGrad);
			ImGui::DragFloat("##gradrange", &mGradRange, 0.01f, 0.001f, 100.0f, "Grad: %f");
			ImGui::Checkbox("Log", &mGradVisLog);
			ImGui::Checkbox("Accumulate", &mGradVisAcc);
			ImGui::Checkbox("Weights", &mGradVisWeights);
		}
		if (mShowFDGrad) {
			ImGui::Checkbox("Show Fd Gradient", &mShowFDGrad);
			ImGui::DragFloat("##gradrange", &mGradRange, 0.01f, 0.001f, 100.0f, "Grad: %f");
			ImGui::Checkbox("Log", &mGradVisLog);
			ImGui::Checkbox("Accumulate", &mGradVisAcc);
			ImGui::Checkbox("Weights", &mGradVisWeights);
		}
#ifdef IALT_USE_SPHERICAL_HARMONICS
		ImGui::Checkbox("Show SH", &mShowSH);
		ImGui::Checkbox("Show Interpolated SH", &mShowInterpolatedSH);
		if(mShowSH || mShowInterpolatedSH) ImGui::DragFloat("##SHS", &mShowSHSize, 0.001f, 0.001f, 5.0, "SH Radius: %.3f");

#endif
		if(ImGui::Checkbox("Use PT for Fwd", &fwdPT))
		{
			mLto.useForwardPT(fwdPT);
			mLto.forward(mLto.getCurrentParams(), &mData->mRadianceBufferCopy);
		}
		if (ImGui::Checkbox("Use PT for Bwd", &bwdPT))
		{
			mLto.useBackwardPT(bwdPT);
		}
		ImGui::Text("Cull Mode:"); ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##cmcombo", mCullMode[mActiveCullMode].c_str(), ImGuiComboFlags_NoArrowButton)) {
			for (uint32_t i = 0; i < mCullMode.size(); i++) {
				const bool isSelected = (i == mActiveCullMode);
				if (ImGui::Selectable(mCullMode[i].c_str(), isSelected)) mActiveCullMode = i;
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::End();
	}

	if (aUiConf->scene->getSelection().reference && aUiConf->scene->getSelection().reference->type == Ref::Type::Light) {

		if( mLto.getLightOptParams().count(static_cast<tamashii::RefLight*>( aUiConf->scene->getSelection().reference.get() ))==0){ 
			mLto.getLightOptParams()[static_cast<tamashii::RefLight*>( aUiConf->scene->getSelection().reference.get())] = LightOptParams();
		}
		if (ImGui::Begin("Edit", nullptr, 0) && mLto.getLightOptParams().count(static_cast<tamashii::RefLight*>( aUiConf->scene->getSelection().reference.get()))>0 )
		{
			bool lightSettingsUpdate = false;
			
			LightOptParams& lp = mLto.getLightOptParams()[static_cast<tamashii::RefLight*>(aUiConf->scene->getSelection().reference.get())];
			ImGui::Separator();
			ImGui::Text("Optimization Parameters");
			ImGui::Text("Position: ");
			ImGui::SameLine();
			bool xyz = lp[LightOptParams::POS_X] && lp[LightOptParams::POS_Y] && lp[LightOptParams::POS_Z];
			if (ImGui::Checkbox("all", &xyz)) {
				lp[LightOptParams::POS_X] = lp[LightOptParams::POS_Y] = lp[LightOptParams::POS_Z] = xyz;
				lightSettingsUpdate |= true;
			}
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("x", &lp[LightOptParams::POS_X]);
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("y", &lp[LightOptParams::POS_Y]);
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("z", &lp[LightOptParams::POS_Z]);

			ImGui::Text("Intensity:");
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("##intensity_opti", &lp[LightOptParams::INTENSITY]);

			if (dynamic_cast<tamashii::SpotLight*>(static_cast<tamashii::RefLight*>(aUiConf->scene->getSelection().reference.get())->light.get()) ||
				dynamic_cast<tamashii::SurfaceLight*>(static_cast<tamashii::RefLight*>(aUiConf->scene->getSelection().reference.get())->light.get()) ||
				dynamic_cast<tamashii::IESLight*>(static_cast<tamashii::RefLight*>(aUiConf->scene->getSelection().reference.get())->light.get())
				) {
				ImGui::Text("Rotation: ");
				ImGui::SameLine();
				bool rxyz = lp[LightOptParams::ROT_X] && lp[LightOptParams::ROT_Y] && lp[LightOptParams::ROT_Z];
				if (ImGui::Checkbox("rall", &rxyz)) {
					lp[LightOptParams::ROT_X] = lp[LightOptParams::ROT_Y] = lp[LightOptParams::ROT_Z] = rxyz;
					lightSettingsUpdate |= true;
				}
				ImGui::SameLine();
				lightSettingsUpdate |= ImGui::Checkbox("rx", &lp[LightOptParams::ROT_X]);
				ImGui::SameLine();
				lightSettingsUpdate |= ImGui::Checkbox("ry", &lp[LightOptParams::ROT_Y]);
				ImGui::SameLine();
				lightSettingsUpdate |= ImGui::Checkbox("rz", &lp[LightOptParams::ROT_Z]);
			}

			if (dynamic_cast<tamashii::SpotLight*>(static_cast<tamashii::RefLight*>(aUiConf->scene->getSelection().reference.get())->light.get())) {
				ImGui::Text("Cone angles: ");
				ImGui::SameLine();
				lightSettingsUpdate |= ImGui::Checkbox("inner", &lp[LightOptParams::CONE_INNER]);
				ImGui::SameLine();
				lightSettingsUpdate |= ImGui::Checkbox("edge", &lp[LightOptParams::CONE_EDGE]);
			}
			ImGui::Text("Color:    ");
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("rgb", &lp[LightOptParams::COLOR_R]);
			lp[LightOptParams::COLOR_G] = lp[LightOptParams::COLOR_B] = lp[LightOptParams::COLOR_R];

			if (lightSettingsUpdate) mLto.exportLightSettings();

			ImGui::Separator();
			ImGui::Text("Gradient Visualization");
			ImGui::Text("Parameter:");
			ImGui::SameLine();
			static const char* gradImageOptions[13] = { "X", "Y", "Z", "Intensity", "Rot X", "Rot Y", "Rot Z", "Cone Inner", "Cone Edge", "Color R", "Color G", "Color B", "Off"};
			
			{
				uint32_t currentGradImage = (sizeof(gradImageOptions) / sizeof(char*)) - 1;
				if (mGradImageSelection.has_value() && mGradImageSelection.value().first == aUiConf->scene->getSelection().reference.get()) currentGradImage = mGradImageSelection.value().second;
				if (ImGui::BeginCombo("##gradImgCombo", gradImageOptions[currentGradImage], ImGuiComboFlags_NoArrowButton)) {
					for (uint32_t i = 0; i < sizeof(gradImageOptions) / sizeof(char*); i++) {
						const bool isSelected = (i == currentGradImage);
						if (ImGui::Selectable(gradImageOptions[i], isSelected)) {
							currentGradImage = i;
							if (currentGradImage != (sizeof(gradImageOptions) / sizeof(char*)) - 1) {
								mGradImageSelection.emplace(static_cast<RefLight*>(aUiConf->scene->getSelection().reference.get()), static_cast<LightOptParams::PARAMS>(i));
								mShowGrad = true;
								mFDGradImageSelection.reset();
								mShowFDGrad = false;
								mGradVisAcc = false;
							}
							else {
								mGradImageSelection.reset();
								mShowGrad = false;
							}
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			ImGui::Separator();
			ImGui::Text("Finite Difference Visualization");
			ImGui::PushItemWidth(150);
			ImGui::Text("Parameter:");
			ImGui::SameLine();
			uint32_t currentGradImageFD = (sizeof(gradImageOptions) / sizeof(char*)) - 1;
			if (mFDGradImageSelection.has_value() && mFDGradImageSelection.value().first == aUiConf->scene->getSelection().reference.get()) currentGradImageFD = mFDGradImageSelection.value().second;
			if (ImGui::BeginCombo("##fdgradImgCombo", gradImageOptions[currentGradImageFD], ImGuiComboFlags_NoArrowButton)) {
				for (uint32_t i = 0; i < sizeof(gradImageOptions) / sizeof(char*); i++) {
					const bool isSelected = (i == currentGradImageFD);
					if (ImGui::Selectable(gradImageOptions[i], isSelected)) {
						currentGradImageFD = i;
						if (currentGradImageFD != (sizeof(gradImageOptions) / sizeof(char*)) - 1) {
							mFDGradImageSelection.emplace(static_cast<RefLight*>(aUiConf->scene->getSelection().reference.get()), static_cast<LightOptParams::PARAMS>(i));
							mLto.fillFiniteDiffRadianceBuffer(mFDGradImageSelection.value().first, mFDGradImageSelection.value().second, mFdGradH, &mData->mFdRadianceBufferCopy, &mData->mFd2RadianceBufferCopy);
							mShowFDGrad = true;
							mGradImageSelection.reset();
							mShowGrad = false;
							mGradVisAcc = false;
						}
						else {
							mFDGradImageSelection.reset();
							mShowFDGrad = false;
						}
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
			if(ImGui::DragFloat("##stepsize", &mFdGradH, 0.01f, 0.01f, 1.0f, "h: %.3g"))
			{
				if (mFDGradImageSelection.has_value())
				{
					mLto.fillFiniteDiffRadianceBuffer(mFDGradImageSelection.value().first, mFDGradImageSelection.value().second, mFdGradH, &mData->mFdRadianceBufferCopy, &mData->mFd2RadianceBufferCopy);
				}
			}
			ImGui::PopItemWidth();


			/*ImGui::Separator();
			static bool shoft = true;
			static float h = 0.1f;
			if (ImGui::Button("Capture", ImVec2(70, 0.0f)))
			{
			}
			ImGui::SameLine();
			ImGui::PushItemWidth(60);
			ImGui::DragFloat("##steps", &h, 0.01f, 0.0, 1.0, "h: %.3g");
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Checkbox("##showfd", &shoft);*/

			ImGui::End();
		}
	}
	if (aUiConf->scene->getSelection().reference && aUiConf->scene->getSelection().reference->type == Ref::Type::Model) {

		auto refModel = static_cast<RefModel*>(aUiConf->scene->getSelection().reference.get());
		auto front = refModel->refMeshes.begin();
		std::advance(front, aUiConf->scene->getSelection().meshOffset);

		if (mLto.getLightOptParams().count(front->get()) == 0) { 
			mLto.getLightOptParams()[front->get()] = LightOptParams();
		}
		if (ImGui::Begin("Edit", nullptr, 0) && mLto.getLightOptParams().count(front->get()) > 0)
		{
			bool lightSettingsUpdate = false;
			
			LightOptParams& lp = mLto.getLightOptParams()[front->get()];
			ImGui::Separator();
			ImGui::Text("Optimization Parameters");

			ImGui::Text("Emission Strength:");
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("##emission_opti", &lp[LightOptParams::INTENSITY]);

			ImGui::Text("Emission Color:   ");
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("##emission_rgb_opti", &lp[LightOptParams::COLOR_R]);
			lp[LightOptParams::COLOR_G] = lp[LightOptParams::COLOR_B] = lp[LightOptParams::COLOR_R];

			ImGui::Text("Emission Texture:");
			ImGui::SameLine();
			lightSettingsUpdate |= ImGui::Checkbox("##emission_tex_opti", &lp[LightOptParams::EMISSIVE_TEXTURE]);

			if (lightSettingsUpdate) mLto.exportLightSettings();

			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			

			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			

			
			
			
			
			
			
			
			
			
			

			ImGui::End();
	}
	}
	if (aUiConf->draw_info->mDrawMode) {
		aUiConf->draw_info->mTarget = DrawInfo::Target::CUSTOM;
#ifdef IALT_USE_SPHERICAL_HARMONICS
		if (ImGui::Begin("Draw", nullptr, 0))
		{
			ImGui::Checkbox("Set SH", &mDrawSetSH);
			ImGui::End();
		}
#endif
	}
}

void InteractiveAdjointLightTracing::waitForNextFrame()
{
	std::mutex mutex;
	std::unique_lock<std::mutex> lck(mutex);
	mNextFrameCV.wait(lck);
}

void InteractiveAdjointLightTracing::startOptimizerThread(std::function<void(InteractiveAdjointLightTracing*, LightTraceOptimizer*, rvk::Buffer*, unsigned int, float, int) > funcToRun)
{
	if (mOptimizerThread.joinable()) {
		spdlog::warn("Optimizer still running. Waiting for it to stop...");
		mOptimizerThread.join();
	}

	mOptimizerThread = std::thread(funcToRun, this, &mLto, &mData->mRadianceBufferCopy, mOptimizerChoice, mOptimizerStepSize, mOptMaxIters.value());
}

void InteractiveAdjointLightTracing::runForward(const Eigen::Map<Eigen::VectorXd>& params)
{
	if (!mData.has_value()) {
		throw std::runtime_error{ "IALT has no data member set" };
	}

	Eigen::VectorXd p = params;
	mLto.forward(p, &mData->mRadianceBufferCopy);
}

double InteractiveAdjointLightTracing::runBackward(Eigen::VectorXd& derivParams)
{
	return mLto.backward(derivParams);
}

void InteractiveAdjointLightTracing::useCurrentRadianceAsTarget(const bool clearWeights)
{
	if(clearWeights) mLto.setTargetWeights(1);
	mLto.copyRadianceToTarget();
	mLto.buildObjectiveFunction(Common::getInstance().getRenderSystem()->getMainScene().get()->getSceneData());
	mLto.copyTargetToMesh(Common::getInstance().getRenderSystem()->getMainScene().get()->getSceneData());
}

InteractiveAdjointLightTracing::OptimizerResult InteractiveAdjointLightTracing::runOptimizer(const LightTraceOptimizer::Optimizers optimizerType, const float stepSize, int maxIterations)
{
	auto result= mLto.optimize(optimizerType, &mData->mRadianceBufferCopy, stepSize, maxIterations );
	const auto history = mLto.exportHistory();

	return { .history = history, .lastPhi = result.lastPhi };
}

void InteractiveAdjointLightTracing::clearGradVis()
{
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	mData->mDerivVisImageAccumulate.CMD_ClearColor(stc.buffer(), 0, 0, 0, 0);
	stc.end();
}

std::unique_ptr<tamashii::Image> InteractiveAdjointLightTracing::getFrameImage()
{
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	const VkExtent3D extent = mFrameData[mRoot.currentIndex()].mColor.getExtent();
	std::vector<glm::vec4> img_data(extent.width * extent.height);
	mFrameData[mRoot.currentIndex()].mColor.STC_DownloadData2D(&stc, extent.width, extent.height, 16, img_data.data());

	const auto img = new tamashii::Image("");
	img->init(extent.width, extent.height, tamashii::Image::Format::RGBA32_FLOAT, img_data.data());
	return std::unique_ptr<tamashii::Image>{img};
}

std::unique_ptr<tamashii::Image> InteractiveAdjointLightTracing::getGradImage()
{

	SingleTimeCommand stc = mRoot.singleTimeCommand();
	const VkExtent3D extent = mData.value().mDerivVisImageAccumulate.getExtent();

	std::vector<glm::vec4> img_data(extent.width * extent.height);
	mData->mDerivVisImageAccumulate.STC_DownloadData2D(&stc, extent.width, extent.height, 16, img_data.data());
	
	
	

	const auto img = new tamashii::Image("");
	img->init(extent.width, extent.height, tamashii::Image::Format::RGBA32_FLOAT, img_data.data());
	return std::unique_ptr<tamashii::Image>{img};
}


