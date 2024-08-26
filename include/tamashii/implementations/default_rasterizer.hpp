#pragma once
#include <tamashii/core/render/render_backend_implementation.hpp>
#include <rvk/rvk.hpp>

#include <tamashii/renderer_vk/convenience/texture_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/geometry_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/material_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/light_to_gpu.hpp>
#include <tamashii/renderer_vk/render_backend.hpp>

T_BEGIN_NAMESPACE
class DefaultRasterizer final : public RenderBackendImplementation {
public:
						DefaultRasterizer(const tamashii::VulkanRenderRoot& aRoot) :
							mRoot{ aRoot }, mGpuTd{ &aRoot.device }, mGpuGd{ &aRoot.device }, mGpuMd{ &aRoot.device },
							mGpuLd{ &aRoot.device }, mActiveCullMode{ 0 }, mActiveDisplayMode{ 0 }, mRgbOrAlpha{ false }
						{
							mCullMode = { "None", "Front", "Back", "Front and Back" };
							mDisplayMode = { "Default", "Vertex Color", "Metallic", "Roughness", "Normal Map", "Occlusion", "Light Map"};
						}
						~DefaultRasterizer() override = default;
						DefaultRasterizer(const DefaultRasterizer&) = delete;
						DefaultRasterizer& operator=(const DefaultRasterizer&) = delete;
						DefaultRasterizer(DefaultRasterizer&&) = delete;
						DefaultRasterizer& operator=(DefaultRasterizer&&) = delete;

	const char*			getName() override { return "Rasterizer"; }
						
	void				windowSizeChanged(uint32_t aWidth, uint32_t aHeight) override;
	void				entityAdded(const Ref& aRef) override;
	void				entityRemoved(const Ref& aRef) override;
	bool				drawOnMesh(const DrawInfo* aDrawInfo) override;

						
	void				prepare(RenderInfo_s& aRenderInfo) override;
	void				destroy() override;

	void				prepareTBNVis();
	void				prepareBoundingBoxVis();
	void				prepareCircleVis();

						
	void				sceneLoad(SceneBackendData aScene) override;
	void				sceneUnload(SceneBackendData aScene) override;

						
    void				drawView(ViewDef_s* aViewDef) override;
    void				drawUI(UiConf_s* aUiConf) override;

	void				drawTBNVis(const rvk::CommandBuffer* aCb, const ViewDef_s* aViewDef);
	void				drawBoundingBoxVis(const rvk::CommandBuffer* aCb, const ViewDef_s* aViewDef);
	void				drawCircleVis(const rvk::CommandBuffer* aCb, const ViewDef_s* aViewDef);

private:

	VulkanRenderRoot										mRoot;
	
	TextureDataVulkan										mGpuTd;
	GeometryDataVulkan										mGpuGd;
	MaterialDataVulkan										mGpuMd;
	LightDataVulkan											mGpuLd;

	
	struct GpuData {
		explicit GpuData(rvk::LogicalDevice& aDevice, uint32_t aFrameCount) : shader{ &aDevice },
			pipelineCullNone{ &aDevice }, pipelineCullFront{ &aDevice }, pipelineCullBack{ &aDevice }, pipelineCullBoth{ &aDevice },
			pipelineWireframeCullNone{ &aDevice }, pipelineWireframeCullFront{ &aDevice }, pipelineWireframeCullBack{ &aDevice },
			pipelineWireframeCullBoth{ &aDevice }, currentPipeline{ nullptr },
			shaderTbnVis{ &aDevice }, pipelineTbnVis{ &aDevice }, shaderBoundingBoxVis{ &aDevice }, pipelineBoundingBoxVis{ &aDevice },
			shaderCircleVis{ &aDevice }, pipelineCircleVis{ &aDevice }
		{ frameData.resize(aFrameCount, GpuFrameData{ aDevice }); }

		rvk::RShader										shader;
		rvk::RPipeline										pipelineCullNone;
		rvk::RPipeline										pipelineCullFront;
		rvk::RPipeline										pipelineCullBack;
		rvk::RPipeline										pipelineCullBoth;
		rvk::RPipeline										pipelineWireframeCullNone;
		rvk::RPipeline										pipelineWireframeCullFront;
		rvk::RPipeline										pipelineWireframeCullBack;
		rvk::RPipeline										pipelineWireframeCullBoth;
		rvk::RPipeline										*currentPipeline;

		rvk::RShader										shaderTbnVis;
		rvk::RPipeline										pipelineTbnVis;
		rvk::RShader										shaderBoundingBoxVis;
		rvk::RPipeline										pipelineBoundingBoxVis;
		rvk::RShader										shaderCircleVis;
		rvk::RPipeline										pipelineCircleVis;

		
		struct GpuFrameData {
			explicit GpuFrameData(rvk::LogicalDevice& aDevice) : globalDescriptor{ &aDevice }, tbnVisDescriptor{ &aDevice },
				globalUniformBuffer{ &aDevice }, color{ &aDevice }, depth{ &aDevice } {}
			
			rvk::Descriptor									globalDescriptor;
			rvk::Descriptor									tbnVisDescriptor;
			
			rvk::Buffer										globalUniformBuffer;
			
			rvk::Image										color;
			rvk::Image										depth;
		};
		std::vector<GpuFrameData>							frameData;
	};
	std::unique_ptr<GpuData>								mData;


	bool													mShade = false;
	bool													mWireframe = false;
	bool													mUseLightMaps = false;
	bool													mMulVertexColors = false;
	float													mDitherStrength = 1.0f;
	bool													mShowWireframe = false;
	bool													mShowTbn = false;
	bool													mShowModelBb = false;
	bool													mShowMeshBb = false;
	float													mTbnScale = 0.01f;
	float													mTbnDepthBiasConstant = 0.0f;
	float													mTbnDepthBiasSlope = 0.0f;

	std::vector<std::string>								mCullMode;
	uint32_t												mActiveCullMode;
	std::vector<std::string>								mDisplayMode;
	uint32_t												mActiveDisplayMode;
	bool													mRgbOrAlpha;
};
T_END_NAMESPACE