#pragma once
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/render/render_backend_implementation.hpp>
#include <rvk/rvk.hpp>

#include <tamashii/renderer_vk/convenience/texture_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/geometry_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/light_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/material_to_gpu.hpp>
#include <tamashii/renderer_vk/render_backend.hpp>

T_BEGIN_NAMESPACE
class DefaultPathTracer final : public RenderBackendImplementation {
public:

	static ccli::Var<uint32_t> spp;
	static ccli::Var<uint32_t> spf;
	static ccli::Var<int32_t> rrpt;
	static ccli::Var<int32_t> max_depth;
	static ccli::Var<bool> env_lighting;
	static ccli::Var<std::string> output_filename;

	enum class Integrator : uint32_t
	{
		PathBRDF = 0,
		PathHalf = 1,
		PathMIS = 2
	};

						DefaultPathTracer(const tamashii::VulkanRenderRoot& aRoot) :
							mRoot{ aRoot }, mGpuTd{&aRoot.device }, mGpuCd{ &aRoot.device }, mGpuBlas{ &aRoot.device }, mGpuLd{ &aRoot.device }, mGpuMd{ &aRoot.device }
						{
							mGlobalUbo.dither_strength = 1.0f;
							mGlobalUbo.pixelSamplesPerFrame = spf.value();
							mGlobalUbo.accumulatedFrames = 0;

							mGlobalUbo.fr_tmin = 0.2f;
							mGlobalUbo.fr_tmax = 10000.0f;
							mGlobalUbo.br_tmin = 0.001f;
							mGlobalUbo.br_tmax = 10000.0f;
							mGlobalUbo.sr_tmin = 0.001f;
							mGlobalUbo.sr_tmax_offset = -0.01f; 

							mGlobalUbo.pixel_filter_type = 0;
							mGlobalUbo.pixel_filter_width = 1.0f;
							mGlobalUbo.pixel_filter_extra = glm::vec2{ 2.0f };

							mGlobalUbo.exposure_film = 1;

							mGlobalUbo.exposure_tm = 0;
							mGlobalUbo.gamma_tm = 1;
							mGlobalUbo.rrpt = rrpt.value();
							mGlobalUbo.clamp_direct = 0;
							mGlobalUbo.clamp_indirect = 0;
							mGlobalUbo.filter_glossy = 0;
							mGlobalUbo.light_geometry = true;
							mEnvLight = glm::vec4{ var::bg.value()[0], var::bg.value()[1], var::bg.value()[2], 255.f } / 255.0f;

							mGlobalUbo.max_depth = max_depth.value();
							mGlobalUbo.env_shade = env_lighting.value();

							mGlobalUbo.sampling_strategy = 1;
							mSampling = {"BRDF", "BRDF + Light (0.5)", "BRDF + Light (MIS)"};
							mPixelFilters = {"Box", "Triangle", "Gaussian", "Blackman-Harris", "Mitchell-Netravali"};
							mToneMappings = {
								"None", "Reinhard", "Reinhard Ext", "Uncharted2", "Uchimura", "Lottes", "Filmic", "ACES"
							};
							mCullMode = { "None", "Front", "Back" };
						}
						~DefaultPathTracer() override = default;
						DefaultPathTracer(const DefaultPathTracer&) = delete;
						DefaultPathTracer& operator=(const DefaultPathTracer&) = delete;
						DefaultPathTracer(DefaultPathTracer&&) = delete;
						DefaultPathTracer& operator=(DefaultPathTracer&&) = delete;

	const char*			getName() override { return "Path Tracer"; }

	void				windowSizeChanged(uint32_t aWidth, uint32_t aHeight) override;
	void				screenshot(const std::string& aFilename) override;
	void				entityAdded(const Ref& aRef) override;

						
	void				prepare(RenderInfo_s& aRenderInfo) override;
	void				destroy() override;

						
	void				sceneLoad(SceneBackendData aScene) override;
	void				sceneUnload(SceneBackendData aScene) override;

						
    void				drawView(ViewDef_s* aViewDef) override;
    void				drawUI(UiConf_s* aUiConf) override;

private:
	VulkanRenderRoot										mRoot;
	uint32_t												mLastIndex;
	
	TextureDataVulkan										mGpuTd;
	CubemapTextureData_GPU									mGpuCd;
	GeometryDataBlasVulkan									mGpuBlas;
	LightDataVulkan											mGpuLd;
	std::vector<GeometryDataTlasVulkan>						mGpuTlas;	
	std::vector<SceneUpdateInfo>							mUpdates;	
	MaterialDataVulkan										mGpuMd;

	
	struct GpuData {
		explicit GpuData(rvk::LogicalDevice& aDevice) : rtshader(&aDevice), rtpipeline(&aDevice), rtImageAccumulate(&aDevice),
			rtImageAccumulateCount(&aDevice), cacheImage(&aDevice) {}
		rvk::RTShader										rtshader;
		rvk::RTPipeline										rtpipeline;

		rvk::Image											rtImageAccumulate;
		rvk::Image											rtImageAccumulateCount;
		rvk::Image											cacheImage;
	};
	
	struct GpuFrameData {
		explicit GpuFrameData(rvk::LogicalDevice& aDevice) : globalDescriptor(&aDevice), globalUniformBuffer(&aDevice),
				rtImage(&aDevice), debugImage(&aDevice) {}
		
		rvk::Descriptor										globalDescriptor;
		
		rvk::Buffer											globalUniformBuffer;
		rvk::Image											rtImage;
		rvk::Image											debugImage;
	};
	std::unique_ptr<GpuData>								mData;
	std::vector<GpuFrameData>								mFrameData;

	
	
	#include "../../../assets/shader/default_path_tracer/defines.h"
	GlobalUbo_s												mGlobalUbo = {};

	float													mEnvLightIntensity = 1.0f;
	glm::vec3												mEnvLight = {};
	bool													mRecalculate = false;
	bool													mDebugImage = false;
	bool													mDebugOutput = false;
	bool													mShowCache = false;

	uint32_t												mActiveCullMode = 0;
	std::vector<std::string>								mCullMode;
	std::vector<std::string>								mSampling;
	std::vector<std::string>								mPixelFilters;
	std::vector<std::string>								mToneMappings;

public:
	void													accumulate(const bool b) { mGlobalUbo.accumulate = b; }
	bool													accumulate() const { return mGlobalUbo.accumulate; }
	void													clearAccumulationBuffers();

	[[nodiscard]] int getDepth() const { return mGlobalUbo.max_depth; }
	void setDepth(const int depth) { mGlobalUbo.max_depth = depth; }

	[[nodiscard]] Integrator getIntegrator() const { return static_cast<Integrator>(mGlobalUbo.sampling_strategy); }
	void setIntegrator(const Integrator integrator) { mGlobalUbo.sampling_strategy = static_cast<unsigned>(integrator); }

	tamashii::Image getAccumulationImage(bool srgb, bool alpha) const;
};
T_END_NAMESPACE