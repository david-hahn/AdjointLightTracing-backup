#include <tamashii/implementations/default_path_tracer.hpp>
#include <imgui.h>

#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/common/input.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/platform/filewatcher.hpp>

#include <glm/gtc/color_space.hpp>

T_USE_NAMESPACE
RVK_USE_NAMESPACE


ccli::Var<uint32_t> DefaultPathTracer::spp("", "spp", 0, ccli::Flag::ConfigRead, "Number of samples to reach before quitting (0 for unlimited)");
ccli::Var<uint32_t> DefaultPathTracer::spf("", "spf", 1, ccli::Flag::ConfigRead, "Number of samples per frame");
ccli::Var<int32_t> DefaultPathTracer::rrpt("", "rrpt", 5, ccli::Flag::ConfigRead, "Depth at which russian roulette path termination starts (-1 for off)");
ccli::Var<int32_t> DefaultPathTracer::max_depth("", "max_depth", -1, ccli::Flag::ConfigRead, "Set number of indirect bounces (-1 for unlimited)");
ccli::Var<bool> DefaultPathTracer::env_lighting("", "env_lighting", true, ccli::Flag::ConfigRead, "Set environment lighting");
ccli::Var<std::string> DefaultPathTracer::output_filename("", "output_filename", "", ccli::Flag::None, "Ray traced results output name");


constexpr uint32_t MAX_GLOBAL_IMAGE_SIZE = 256;
constexpr uint32_t MAX_GLOBAL_CUBE_IMAGE_SIZE = 4;
constexpr uint32_t MAX_MATERIAL_SIZE = (3 * 1024);

constexpr VkFormat ACCUMULATION_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr VkFormat COUNT_FORMAT = VK_FORMAT_R32_SFLOAT;

void DefaultPathTracer::windowSizeChanged(const uint32_t aWidth, const uint32_t aHeight) {
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	mGlobalUbo.accumulatedFrames = 0;
	
	mData->rtImageAccumulate.destroy();
	mData->rtImageAccumulateCount.destroy();
	
	mData->rtImageAccumulate.createImage2D(aWidth, aHeight, ACCUMULATION_FORMAT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	mData->rtImageAccumulate.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
	mData->rtImageAccumulateCount.createImage2D(aWidth, aHeight, COUNT_FORMAT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	mData->rtImageAccumulateCount.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
	
	mData->cacheImage.destroy();
	mData->cacheImage.createImage2D(aWidth, aHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	mData->cacheImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);


	for (uint32_t frameIndex = 0; frameIndex < mRoot.frameCount(); frameIndex++) {
		GpuFrameData& frameData = mFrameData[frameIndex];

		frameData.rtImage.destroy();
		frameData.debugImage.destroy();
		
		frameData.rtImage.createImage2D(aWidth, aHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		frameData.rtImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
		frameData.debugImage.createImage2D(aWidth, aHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		frameData.debugImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_RT_OUT_IMAGE_BINDING, &frameData.rtImage);
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_RT_ACC_IMAGE_BINDING, &mData->rtImageAccumulate);
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_RT_ACC_C_IMAGE_BINDING, &mData->rtImageAccumulateCount);
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_DEBUG_IMAGE_BINDING, &frameData.debugImage);
		if (mGpuTlas[frameIndex].size()) frameData.globalDescriptor.update();
	}
	stc.end();
}

void DefaultPathTracer::screenshot(const std::string& aFilename)
{
	std::filesystem::path p(aFilename);
	if (!p.has_extension()) p = aFilename + ".png";
	const std::string ext = p.extension().string();

	SingleTimeCommand stc = mRoot.singleTimeCommand();
	if (ext == ".png" || !p.has_extension()) {
		const VkExtent3D extent = mFrameData[mRoot.previousIndex()].rtImage.getExtent();
		std::vector<glm::u8vec4> data_uint8(extent.width * extent.height);
		mFrameData[mRoot.previousIndex()].rtImage.STC_DownloadData2D(&stc, extent.width, extent.height, 4, data_uint8.data());
		std::vector<glm::u8vec4> data_image;
		data_image.reserve(extent.width * extent.height);
		for (const auto v : data_uint8) data_image.emplace_back(v.z, v.y, v.x, v.w);
		io::Export::save_image_png_8_bit(p.string(), extent.width, extent.height, 4, reinterpret_cast<uint8_t*>(data_image.data()));
	}
	else if (ext == ".exr") {
		const VkExtent3D extent = mData->rtImageAccumulate.getExtent();
		std::vector<glm::vec4> dataImage(extent.width * extent.height);
		mData->rtImageAccumulate.STC_DownloadData2D(&stc, extent.width, extent.height, 16, dataImage.data());
		for (auto& v : dataImage) v = (v / static_cast<float>(mGlobalUbo.accumulatedFrames));
		io::Export::save_image_exr(p.string(), extent.width, extent.height, reinterpret_cast<float*>(dataImage.data()), 4, { 2, 1, 0 } /*B G R*/);
	}
	else {
		spdlog::error("Export image format not supported");
		return;
	}
	spdlog::info("Impl-Screenshot saved: {}", p.string());
}

void DefaultPathTracer::entityAdded(const Ref& aRef)
{
	if (aRef.type == Ref::Type::Light) {
		
		const_cast<RefLight&>(dynamic_cast<const RefLight&>(aRef)).mask = 0x1;
	}
}


void DefaultPathTracer::prepare(RenderInfo_s& aRenderInfo) {
	mGpuTd.prepare(rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::ANY_HIT, MAX_GLOBAL_IMAGE_SIZE);
	mGpuCd.prepare(rvk::Shader::Stage::RAYGEN, MAX_GLOBAL_CUBE_IMAGE_SIZE);
	mGpuTlas.resize(mRoot.frameCount(), &mRoot.device);
	mUpdates.resize(mRoot.frameCount());
	mGpuMd.prepare(rvk::Buffer::Use::STORAGE, MAX_MATERIAL_SIZE, true);
	mGpuLd.prepare(rvk::Buffer::Use::STORAGE);

	mData = std::make_unique<GpuData>(mRoot.device);
	mFrameData.resize(mRoot.frameCount(), GpuFrameData(mRoot.device));

	
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	mData->rtImageAccumulate.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, ACCUMULATION_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
	mData->rtImageAccumulate.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
	mData->rtImageAccumulateCount.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, COUNT_FORMAT, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
	mData->rtImageAccumulateCount.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
	
	mData->cacheImage.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, VK_FORMAT_B8G8R8A8_UNORM, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
	mData->cacheImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);

	
	for (uint32_t frameIndex = 0; frameIndex < mRoot.frameCount(); frameIndex++) {
		GpuFrameData& frameData = mFrameData[frameIndex];

		
		frameData.rtImage.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, VK_FORMAT_B8G8R8A8_UNORM, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
		frameData.rtImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
		
		frameData.debugImage.createImage2D(aRenderInfo.targetSize.x, aRenderInfo.targetSize.y, VK_FORMAT_B8G8R8A8_UNORM, rvk::Image::Use::DOWNLOAD | rvk::Image::Use::UPLOAD | rvk::Image::Use::STORAGE);
		frameData.debugImage.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_GENERAL);
		
		frameData.globalUniformBuffer.create(rvk::Buffer::Use::UNIFORM, sizeof(GlobalUbo_s), rvk::Buffer::Location::HOST_COHERENT);
		frameData.globalUniformBuffer.mapBuffer();

		
		
		frameData.globalDescriptor.reserve(8);
		frameData.globalDescriptor.addUniformBuffer(GLOBAL_DESC_UBO_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.globalDescriptor.addStorageBuffer(GLOBAL_DESC_GEOMETRY_DATA_BINDING, rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::ANY_HIT);
		frameData.globalDescriptor.addStorageBuffer(GLOBAL_DESC_INDEX_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::ANY_HIT);
		frameData.globalDescriptor.addStorageBuffer(GLOBAL_DESC_VERTEX_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::ANY_HIT);
		frameData.globalDescriptor.addStorageBuffer(GLOBAL_DESC_MATERIAL_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::ANY_HIT);
		frameData.globalDescriptor.addStorageBuffer(GLSL_GLOBAL_LIGHT_DATA_BINDING, rvk::Shader::Stage::RAYGEN | rvk::Shader::Stage::INTERSECTION);
		frameData.globalDescriptor.addAccelerationStructureKHR(GLOBAL_DESC_AS_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.globalDescriptor.addStorageImage(GLSL_GLOBAL_RT_OUT_IMAGE_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.globalDescriptor.addStorageImage(GLSL_GLOBAL_RT_ACC_IMAGE_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.globalDescriptor.addStorageImage(GLSL_GLOBAL_RT_ACC_C_IMAGE_BINDING, rvk::Shader::Stage::RAYGEN);
		frameData.globalDescriptor.addStorageImage(GLSL_GLOBAL_DEBUG_IMAGE_BINDING, rvk::Shader::Stage::RAYGEN);

		
		frameData.globalDescriptor.setBuffer(GLOBAL_DESC_UBO_BINDING, &frameData.globalUniformBuffer);
		frameData.globalDescriptor.setBuffer(GLOBAL_DESC_MATERIAL_BUFFER_BINDING, mGpuMd.getMaterialBuffer());
		frameData.globalDescriptor.setBuffer(GLSL_GLOBAL_LIGHT_DATA_BINDING, mGpuLd.getLightBuffer());
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_RT_OUT_IMAGE_BINDING, &frameData.rtImage);
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_RT_ACC_IMAGE_BINDING, &mData->rtImageAccumulate);
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_RT_ACC_C_IMAGE_BINDING, &mData->rtImageAccumulateCount);
		frameData.globalDescriptor.setImage(GLSL_GLOBAL_DEBUG_IMAGE_BINDING, &frameData.debugImage);
		frameData.globalDescriptor.finish(false);
	}
	stc.end();
	
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, "assets/shader/default_path_tracer/path_tracer.rgen", { "GLSL", "SAMPLE_BRDF" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, "assets/shader/default_path_tracer/path_tracer.rgen", { "GLSL", "SAMPLE_LIGHTS" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, "assets/shader/default_path_tracer/path_tracer.rgen", { "GLSL", "SAMPLE_LIGHTS_MIS" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::MISS, "assets/shader/default_path_tracer/path_tracer.rmiss", { "GLSL" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::CLOSEST_HIT, "assets/shader/default_path_tracer/path_tracer.rchit", { "GLSL" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::ANY_HIT, "assets/shader/default_path_tracer/path_tracer.rahit", { "GLSL" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::INTERSECTION, "assets/shader/default_path_tracer/sphere.rint", { "GLSL" });
	
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::MISS, "assets/shader/default_path_tracer/shadow_ray.rmiss", { "GLSL" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::CLOSEST_HIT, "assets/shader/default_path_tracer/shadow_ray.rchit", { "GLSL" });
	mData->rtshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::ANY_HIT, "assets/shader/default_path_tracer/shadow_ray.rahit", { "GLSL" });
	mData->rtshader.addGeneralShaderGroup("assets/shader/default_path_tracer/path_tracer.rmiss"); 
	mData->rtshader.addGeneralShaderGroup("assets/shader/default_path_tracer/shadow_ray.rmiss"); 
	mData->rtshader.addGeneralShaderGroup(0); 
	mData->rtshader.addGeneralShaderGroup(1); 
	mData->rtshader.addGeneralShaderGroup(2); 

	mData->rtshader.addHitShaderGroup("assets/shader/default_path_tracer/path_tracer.rchit", "assets/shader/default_path_tracer/path_tracer.rahit"); 
	mData->rtshader.addProceduralShaderGroup("assets/shader/default_path_tracer/path_tracer.rchit", "", "assets/shader/default_path_tracer/sphere.rint"); 

	mData->rtshader.addHitShaderGroup("assets/shader/default_path_tracer/shadow_ray.rchit", "assets/shader/default_path_tracer/shadow_ray.rahit"); 
	mData->rtshader.addProceduralShaderGroup("assets/shader/default_path_tracer/shadow_ray.rchit", "", "assets/shader/default_path_tracer/sphere.rint"); 
	mData->rtshader.finish();

	tamashii::FileWatcher::getInstance().watchFile("assets/shader/default_path_tracer/path_tracer.rgen", [this]() { mData->rtshader.reloadShader({ 0,1,2 }); });
	

	mData->rtpipeline.setShader(&mData->rtshader);
	mData->rtpipeline.addDescriptorSet({ mGpuTd.getDescriptor(), &mFrameData[0].globalDescriptor, mGpuCd.getDescriptor() });
	mData->rtpipeline.finish();
}
void DefaultPathTracer::destroy() {
	mGpuTd.destroy();
	mGpuCd.destroy();
	mGpuMd.destroy();
	mGpuLd.destroy();

	tamashii::FileWatcher::getInstance().removeFile("assets/shader/default_path_tracer/path_tracer.rgen");

	mFrameData.clear();
	mData.reset();
}


void DefaultPathTracer::sceneLoad(SceneBackendData aScene) {
	for (const auto& l : aScene.refLights) l->mask = 0x1;
	const GeometryDataVulkan::SceneInfo_s sinfo = GeometryDataVulkan::getSceneGeometryInfo(aScene);
	if (!sinfo.mMeshCount) {
		mGpuBlas.prepare();
		for (GeometryDataTlasVulkan& c : mGpuTlas) c.prepare(100, rvk::Buffer::Use::STORAGE, 100);
	}
	else {
		mGpuBlas.prepare(sinfo.mIndexCount, sinfo.mVertexCount);
		for (GeometryDataTlasVulkan& c : mGpuTlas) c.prepare(sinfo.mInstanceCount + 100, rvk::Buffer::Use::STORAGE, sinfo.mGeometryCount + 100);
	}

	SingleTimeCommand stc = mRoot.singleTimeCommand();
	mGpuTd.loadScene(&stc, aScene);

	
	
	mGpuCd.loadScene(&stc, { });
	mGpuBlas.loadScene(&stc, aScene);
	mGpuMd.loadScene(&stc, aScene, &mGpuTd);
	mGpuLd.loadScene(&stc, aScene, &mGpuTd, &mGpuBlas);
	for (GeometryDataTlasVulkan& tlas : mGpuTlas) tlas.loadScene(&stc, aScene, &mGpuBlas, &mGpuMd, mGlobalUbo.light_geometry ? &mGpuLd : nullptr);
	for (uint32_t idx = 0; idx < mRoot.frameCount(); idx++) {
		mFrameData[idx].globalDescriptor.setBuffer(GLOBAL_DESC_GEOMETRY_DATA_BINDING, mGpuTlas[idx].getGeometryDataBuffer());
		mFrameData[idx].globalDescriptor.setBuffer(GLOBAL_DESC_INDEX_BUFFER_BINDING, mGpuBlas.getIndexBuffer());
		mFrameData[idx].globalDescriptor.setBuffer(GLOBAL_DESC_VERTEX_BUFFER_BINDING, mGpuBlas.getVertexBuffer());
		if (mGpuMd.bufferChanged(false)) mFrameData[idx].globalDescriptor.setBuffer(GLOBAL_DESC_MATERIAL_BUFFER_BINDING, mGpuMd.getMaterialBuffer());

		mFrameData[idx].globalDescriptor.setAccelerationStructureKHR(GLOBAL_DESC_AS_BINDING, mGpuTlas[idx].getTlas());
		mFrameData[idx].globalDescriptor.update();
	}

	
	mGlobalUbo.shade = !aScene.refLights.empty();
	for (auto& model : aScene.models) {
		for (const auto& mesh : *model) {
			if (mesh->getMaterial()->isLight()) {
				mGlobalUbo.shade = true;
				break;
			}
		}
	}
}

void DefaultPathTracer::sceneUnload(SceneBackendData aScene) {
	mGpuTd.unloadScene();
	mGpuCd.unloadScene();
	mGpuBlas.unloadScene();
	for (GeometryDataTlasVulkan& c : mGpuTlas) {
		c.unloadScene();
	}
	mGpuMd.unloadScene();
	mGpuLd.unloadScene();

	for (GeometryDataTlasVulkan& c : mGpuTlas) c.destroy();
	mGpuBlas.destroy();
}


void DefaultPathTracer::drawView(ViewDef_s* aViewDef) {
	if (!output_filename.value().empty()) {
		mGlobalUbo.accumulate = true;
		if (spp.value() == 0) spp.value(100);
		if (static_cast<int>(mGlobalUbo.accumulatedFrames) >= spp.value())
		{
			screenshot(output_filename.value());
			EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT);
			return;
		}
		if (aViewDef->headless) spdlog::info("[{}/{}]", static_cast<int>(mGlobalUbo.accumulatedFrames), spp.value());
	}

	CommandBuffer& cb = mRoot.currentCmdBuffer();
	const uint32_t fi = mRoot.currentIndex();

	const glm::vec4 cc = glm::vec4{ var::bg.value()[0], var::bg.value()[1], var::bg.value()[2], 255.f } / 255.0f;
	if (aViewDef->surfaces.empty()) {
		bool has_lights = false;
		for (const auto a : aViewDef->lights) {
			if (a->light->getType() == Light::Type::POINT || a->light->getType() == Light::Type::IES) { has_lights = true; break; }
		}
		if (!has_lights) {
			mRoot.currentImage().CMD_TransitionImage(&cb, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			cb.cmdBeginRendering(
				{ { &mRoot.currentImage(),
				{{cc.x, cc.y, cc.z, 1.0f}} , RVK_LC, RVK_S2 } },
				{ &mRoot.currentDepthImage(),
				{0.0f, 0}, RVK_LC, RVK_S2 }
			);
			cb.cmdEndRendering();
			mRoot.currentImage().CMD_TransitionImage(&cb, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			return;
		}
	};
	if (aViewDef->updates.any()) for (uint32_t i = 0; i < mRoot.frameCount(); i++) mUpdates[i] = mUpdates[i] | aViewDef->updates;

	
	if (mUpdates[fi].mModelInstances || mUpdates[fi].mMaterials || mUpdates[fi].mLights) {
		
		mUpdates[fi].mModelInstances = mUpdates[fi].mMaterials = false;
		SingleTimeCommand stc = mRoot.singleTimeCommand();
		mGpuMd.update(&stc, aViewDef->scene, &mGpuTd);
		mGpuTlas[fi].update(&cb, &stc, aViewDef->scene, &mGpuBlas, &mGpuMd, mGlobalUbo.light_geometry ? &mGpuLd : nullptr);

		cb.cmdMemoryBarrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
		mFrameData[fi].globalDescriptor.setAccelerationStructureKHR(GLOBAL_DESC_AS_BINDING, mGpuTlas[fi].getTlas());
		mFrameData[fi].globalDescriptor.update();
	}
	if (aViewDef->updates.mImages || aViewDef->updates.mTextures) {
		mRoot.device.waitIdle();
		SingleTimeCommand stc = mRoot.singleTimeCommand();
		mGpuTd.update(&stc, aViewDef->scene);
	}
	if (aViewDef->updates.mLights) {
		SingleTimeCommand stc = mRoot.singleTimeCommand();
		mGpuLd.update(&stc, aViewDef->scene, &mGpuTd, &mGpuBlas);
	}
	if (aViewDef->updates.any() || mRecalculate) {
		mRecalculate = false;
		mGlobalUbo.accumulatedFrames = 0;
		mData->rtImageAccumulate.CMD_ClearColor(&cb, 0, 0, 0, 0);
		mData->rtImageAccumulateCount.CMD_ClearColor(&cb, 0, 0, 0, 0);
	}
	if (mGlobalUbo.accumulate && spp.value() != 0 && static_cast<int>(mGlobalUbo.accumulatedFrames) >= spp.value()) {
		if (mShowCache) rvk::swapchain::CMD_BlitImageToImage(&cb, &mData->cacheImage, &mRoot.currentImage(), VK_FILTER_LINEAR);
		else rvk::swapchain::CMD_BlitImageToImage(&cb, &mFrameData[mLastIndex].rtImage, &mRoot.currentImage(), VK_FILTER_LINEAR);
		return;
	}

	const glm::vec3 bg{ var::bg.value()[0], var::bg.value()[1], var::bg.value()[2] };
	mFrameData[mRoot.currentIndex()].rtImage.CMD_ClearColor(&cb, bg.x / 255.0f, bg.y / 255.0f, bg.z / 255.0f, 1.0f);
	mFrameData[mRoot.currentIndex()].debugImage.CMD_ClearColor(&cb, 0, 0, 0, 1.0f);

	static glm::vec2 debugClickPos = glm::vec2{ 0, 0 };
	if (tamashii::InputSystem::getInstance().wasPressed(tamashii::Input::MOUSE_LEFT)) {
		debugClickPos = tamashii::InputSystem::getInstance().getMousePosAbsolute();
	}

	SingleTimeCommand stl = mRoot.singleTimeCommand();
	stl.begin();

	
	mGlobalUbo.viewMat = aViewDef->view_matrix;
	mGlobalUbo.projMat = aViewDef->projection_matrix;
	mGlobalUbo.inverseViewMat = aViewDef->inv_view_matrix;
	mGlobalUbo.inverseProjMat = aViewDef->inv_projection_matrix;
	mGlobalUbo.viewPos = glm::vec4{ aViewDef->view_pos, 1 };
	mGlobalUbo.viewDir = glm::vec4{ aViewDef->view_dir, 0 };
	mGlobalUbo.debugPixelPosition = debugClickPos;
	mGlobalUbo.cull_mode = mActiveCullMode;
	Common::getInstance().intersectionSettings().mCullMode = static_cast<CullMode>(mActiveCullMode);
	mGlobalUbo.bg[0] = mEnvLight[0] * mEnvLightIntensity; mGlobalUbo.bg[1] = mEnvLight[1] * mEnvLightIntensity; mGlobalUbo.bg[2] = mEnvLight[2] * mEnvLightIntensity; mGlobalUbo.bg[3] = 1;
	mGlobalUbo.size[0] = static_cast<float>(aViewDef->target_size.x); mGlobalUbo.size[1] = static_cast<float>(aViewDef->target_size.y);
	mGlobalUbo.frameIndex = static_cast<float>(aViewDef->frame_index);
	mGlobalUbo.light_count = mGpuLd.getLightCount();
	
	if (mGlobalUbo.accumulate) mGlobalUbo.accumulatedFrames += mGlobalUbo.pixelSamplesPerFrame;
	else mGlobalUbo.accumulatedFrames = mGlobalUbo.pixelSamplesPerFrame;
	mFrameData[mRoot.currentIndex()].globalUniformBuffer.STC_UploadData(&stl, &mGlobalUbo, sizeof(GlobalUbo_s));
	stl.end();

	GpuFrameData& frameData = mFrameData[mRoot.currentIndex()];
	if (!aViewDef->scene.refModels.empty()) {
		mData->rtpipeline.CMD_BindDescriptorSets(&cb, { mGpuTd.getDescriptor(), &frameData.globalDescriptor, mGpuCd.getDescriptor() });
		mData->rtpipeline.CMD_BindPipeline(&cb);
		mData->rtpipeline.CMD_TraceRays(&cb, aViewDef->target_size.x, aViewDef->target_size.y, 1, mGlobalUbo.sampling_strategy);
	}

	if (mShowCache) rvk::swapchain::CMD_BlitImageToImage(&cb, &mData->cacheImage, &mRoot.currentImage(), VK_FILTER_LINEAR);
	else if (mDebugImage) rvk::swapchain::CMD_BlitImageToImage(&cb, &frameData.debugImage, &mRoot.currentImage(), VK_FILTER_LINEAR);
	else rvk::swapchain::CMD_BlitImageToImage(&cb, &frameData.rtImage, &mRoot.currentImage(), VK_FILTER_LINEAR);
	
	mLastIndex = mRoot.currentIndex();
}
void DefaultPathTracer::drawUI(UiConf_s* aUiConf) {
	if (ImGui::Begin("Settings", nullptr, 0))
	{
		ImGui::Separator();
		ImGui::Checkbox("Shade", reinterpret_cast<bool*>(&mGlobalUbo.shade));
		ImGui::Checkbox("Accumulate", reinterpret_cast<bool*>(&mGlobalUbo.accumulate));
		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		int spp_v = spp.value();
		ImGui::DragInt("##spp", &spp_v, 1, 0, std::numeric_limits<int>::max(), "spp: %d");
		spp.value(spp_v);
		ImGui::PopItemWidth();
		ImGui::DragInt("##spf", reinterpret_cast<int*>(&mGlobalUbo.pixelSamplesPerFrame), 1, 1, 1000, "Pixel Samples per Frame: %d");
		if (ImGui::DragInt("##D", &mGlobalUbo.max_depth, 1, -1, 1000, "Max Depth: %d")) mRecalculate = true;


		if (ImGui::CollapsingHeader("Debug", 0)) {
			if (ImGui::Button("Cache Current Image"))
			{
				rvk::image::CMD_CopyImageToImage(&mRoot.currentCmdBuffer(), &mFrameData[mRoot.currentIndex()].rtImage, &mData->cacheImage);
			}
			ImGui::SameLine();
			ImGui::Text("Show:");
			ImGui::SameLine();
			ImGui::Checkbox("##Showcache", &mShowCache);
#ifndef NDEBUG
			ImGui::Checkbox("Debug View", &mDebugImage);
			ImGui::Checkbox("Debug Output", &mDebugOutput);
#endif
		}

		if (ImGui::CollapsingHeader("Environment Light", 0)) {
			if (ImGui::Checkbox("Environment Shading: ", reinterpret_cast<bool*>(&mGlobalUbo.env_shade))) mRecalculate = true;
			if (ImGui::ColorEdit3("Color", &mEnvLight[0], ImGuiColorEditFlags_NoInputs)) mRecalculate = true;
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::DragFloat("##envlintens", &mEnvLightIntensity, 0.01f, 0, std::numeric_limits<float>::max(), "Intensity: %.3f");
			ImGui::PopItemWidth();
		}
		if (ImGui::CollapsingHeader("Sampling", 0)) {
			if (ImGui::BeginCombo("##scombo", mSampling[mGlobalUbo.sampling_strategy].c_str(), ImGuiComboFlags_NoArrowButton)) {
				for (uint32_t i = 0; i < mSampling.size(); i++) {
					const bool isSelected = (i == mGlobalUbo.sampling_strategy);
					if (ImGui::Selectable(mSampling[i].c_str(), isSelected)) {
						mGlobalUbo.sampling_strategy = i;
						mRecalculate = true;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (ImGui::DragFloat("##filter_glossy", &mGlobalUbo.filter_glossy, 0.01f, 0, 10, "Filter Glossy: %.3f")) mRecalculate = true;
			bool mUseLightGeometry = mGlobalUbo.light_geometry;
			if (ImGui::Checkbox("Light Geometry", &mUseLightGeometry)) {
				mGlobalUbo.light_geometry = mUseLightGeometry;
				for (uint32_t i = 0; i < mRoot.frameCount(); i++) mUpdates[i].mModelInstances = true;
			}
		}
		if (ImGui::CollapsingHeader("Clamping", 0)) {
			if (ImGui::DragFloat("##clamp_direct", &mGlobalUbo.clamp_direct, 0.01f, 0, 10, "Direct Light: %.3f")) mRecalculate = true;
			if (ImGui::DragFloat("##clamp_indirect", &mGlobalUbo.clamp_indirect, 0.01f, 0, 10, "Indirect Light: %.3f")) mRecalculate = true;
		}
		if(ImGui::CollapsingHeader("Ray Settings", 0)){
			ImGui::PushItemWidth(64);
			ImGui::Text("			  Tmin	  Tmax");
			ImGui::Text("First Ray: ");
			ImGui::SameLine();
			ImGui::DragFloat("##firstrayTmin", &mGlobalUbo.fr_tmin, 0.01f, 0, mGlobalUbo.fr_tmax, "%g");
			ImGui::SameLine();
			ImGui::DragFloat("##firstrayTmax", &mGlobalUbo.fr_tmax, 1.0, mGlobalUbo.fr_tmin, std::numeric_limits<float>::max(), "%g");
			ImGui::Text("Bounce Ray:");
			ImGui::SameLine();
			ImGui::DragFloat("##bouncerayTmin", &mGlobalUbo.br_tmin, 0.01f, 0, mGlobalUbo.br_tmax, "%g");
			ImGui::SameLine();
			ImGui::DragFloat("##bouncerayTmax", &mGlobalUbo.br_tmax, 1.0, mGlobalUbo.br_tmin, std::numeric_limits<float>::max(), "%g");
			ImGui::Text("			  Tmin     Offset");
			ImGui::Text("Shadow Ray:");
			ImGui::SameLine();
			ImGui::DragFloat("##shadowrayTmin", &mGlobalUbo.sr_tmin, 0.001f, 0.001f, std::numeric_limits<float>::max(), "%g");
			ImGui::SameLine();
			ImGui::DragFloat("##shadowrayTmax", &mGlobalUbo.sr_tmax_offset, 0.001f, 0, 0, "%g");
			ImGui::PopItemWidth();
			ImGui::Text("Cull Mode: "); ImGui::SameLine();
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
			ImGui::DragInt("##rrpt", &mGlobalUbo.rrpt, 1, -1, 1000, "Russian Roulette PT: %d");
		}
		if (ImGui::CollapsingHeader("Pixel Filter", 0)) {
			if (ImGui::BeginCombo("##pfcombo", mPixelFilters[mGlobalUbo.pixel_filter_type].c_str(), ImGuiComboFlags_NoArrowButton)) {
				for (uint32_t i = 0; i < mPixelFilters.size(); i++) {
					const bool isSelected = (i == mGlobalUbo.pixel_filter_type);
					if (ImGui::Selectable(mPixelFilters[i].c_str(), isSelected)) mGlobalUbo.pixel_filter_type = i;
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::DragFloat("##pfwidth", &mGlobalUbo.pixel_filter_width, 0.01f, 0.0f, 10.0f, "Width: %g px");
			if (mGlobalUbo.pixel_filter_type == 2) {
				static float alpha = 2;
				ImGui::DragFloat("##pfalpha", &alpha, 0.01f, 0.01f, 10, "Alpha: %g");
				mGlobalUbo.pixel_filter_extra.x = alpha;
			}
			if (mGlobalUbo.pixel_filter_type == 4) {
				static float B = 1.0f / 3.0f, C = 1.0f/3.0f;
				ImGui::DragFloat("##pfb", &B, 0.01f, 0.01f, 10.0f, "B: %g");
				ImGui::DragFloat("##pfc", &C, 0.01f, 0.01f, 10.0f, "C: %g");
				mGlobalUbo.pixel_filter_extra = glm::vec2(B, C);
			}

		}
		if (ImGui::CollapsingHeader("Film", 0)) {
			ImGui::DragFloat("##exposure_film", &mGlobalUbo.exposure_film, 0.01f, 0.0f, 10.0f, "Exposure: %g");
			ImGui::DragFloat("##dither", &mGlobalUbo.dither_strength, 0.001, 0, 2.0f, "Dither: %.3f");
		}
		if (ImGui::CollapsingHeader("Tone Mapping", 0)) {
			if (ImGui::BeginCombo("##tmcombo", mToneMappings[mGlobalUbo.tone_mapping_type].c_str(), ImGuiComboFlags_NoArrowButton)) {
				for (uint32_t i = 0; i < mToneMappings.size(); i++) {
					const bool is_selected = (i == mGlobalUbo.tone_mapping_type);
					if (ImGui::Selectable(mToneMappings[i].c_str(), is_selected)) mGlobalUbo.tone_mapping_type = i;
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::DragFloat("##exposure_tm", &mGlobalUbo.exposure_tm, 0.01f, -10.0f, 10.0f, "Exposure: %g");
			ImGui::DragFloat("##gamma_tm", &mGlobalUbo.gamma_tm, 0.01f, 0.0f, 5.0f, "Gamma: %g");
		}
		ImGui::End();
	}
	if (ImGui::Begin("Statistics", nullptr, 0))
	{
		ImGui::Separator();
		const bool unbiased = mGlobalUbo.max_depth == -1 && mGlobalUbo.rrpt != 0 && 
			mGlobalUbo.filter_glossy == 0.0f && mGlobalUbo.clamp_direct == 0.0f && mGlobalUbo.clamp_indirect == 0.0f;
		if(unbiased) ImGui::Text("Unbiased Rendering");
		else ImGui::Text("Biased Rendering");
		ImGui::Text("Samples: %d", mGlobalUbo.accumulatedFrames);
		ImGui::End();
	}
}

void DefaultPathTracer::clearAccumulationBuffers()
{
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	mGlobalUbo.accumulatedFrames = 0;
	mData->rtImageAccumulate.CMD_ClearColor(stc.buffer(), 0, 0, 0, 0);
	mData->rtImageAccumulateCount.CMD_ClearColor(stc.buffer(), 0, 0, 0, 0);
	stc.end();
}

tamashii::Image DefaultPathTracer::getAccumulationImage(const bool srgb, const bool alpha) const
{
	SingleTimeCommand stc = mRoot.singleTimeCommand();
	const VkExtent3D extent = mData->rtImageAccumulate.getExtent();
	std::vector<glm::vec4> dataImage(extent.width * extent.height);
	mData->rtImageAccumulate.STC_DownloadData2D(&stc, extent.width, extent.height, 16, dataImage.data());
	for (auto& v : dataImage) {
		v = (v / static_cast<float>(mGlobalUbo.accumulatedFrames));
		if (srgb) v = glm::convertLinearToSRGB(v);
		v.w = 1;
	}

	if (alpha) {
		tamashii::Image img("accumulation");
		img.init(extent.width, extent.height, Image::Format::RGBA32_FLOAT, dataImage.data());
		return img;
	} else {
		std::vector<glm::vec3> dataImageVec3;
		dataImageVec3.reserve(extent.width * extent.height);
		for (auto& v : dataImage) dataImageVec3.emplace_back(v);
		tamashii::Image img("accumulation");
		img.init(extent.width, extent.height, Image::Format::RGB32_FLOAT, dataImageVec3.data());
		return img;
	}
}
