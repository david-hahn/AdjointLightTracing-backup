#pragma once
#include <tamashii/core/render/render_backend_implementation.hpp>
#include <tamashii/renderer_vk/convenience/texture_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/geometry_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/material_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/light_to_gpu.hpp>
#include <tamashii/renderer_vk/render_backend.hpp>
#include <rvk/rvk.hpp>

#include "light_trace_opti.hpp"

#include <thread>

class InteractiveAdjointLightTracing final : public tamashii::RenderBackendImplementation {
public:
	struct OptimizerResult {
		std::deque<Eigen::VectorXd> history;
		double lastPhi;
	};

					InteractiveAdjointLightTracing(const tamashii::VulkanRenderRoot& aRoot) : mRoot{ aRoot }, mLights{nullptr},
						mLto {aRoot}, mSceneLoaded{ false },
						mShowTarget{ false }, mShowAlpha{ false }, mShowAdjointDeriv{ false },
						mAdjointVisRange{ 1 }, mAdjointVisLog{ false }, mShowWireframeOverlay{ false },
						mWireframeColor{ 0.5f }, mShowSH{ false }, mShowInterpolatedSH{ false }, mShowSHSize{ 0.01f },
						mCullMode{ { "None", "Front", "Back" } }, mActiveCullMode{ 2 }, mShowGrad{ false },
						mGradRange{ 1.0f }, mGradVisLog{ false }, mGradVisAcc{ false }, mGradVisWeights{ false }, mShowFDGrad{ false },
						mFdGradH{ 0.5f }, mDrawSetSH{ false }, mOptimizerChoices{ LightTraceOptimizer::optimizerNames }, mOptimizerChoice{ 0 },
						mOptimizerStepSize{ 1.0f } {}

					~InteractiveAdjointLightTracing() override
					{
						if(mOptimizerThread.joinable()) mOptimizerThread.join();
					}
					InteractiveAdjointLightTracing(const InteractiveAdjointLightTracing&) = delete;
					InteractiveAdjointLightTracing& operator=(const InteractiveAdjointLightTracing&) = delete;
					InteractiveAdjointLightTracing(InteractiveAdjointLightTracing&&) = delete;
					InteractiveAdjointLightTracing& operator=(InteractiveAdjointLightTracing&&) = delete;

	const char*		getName() override { return "ialt"; }

	void			windowSizeChanged(uint32_t aWidth, uint32_t aHeight) override;
	bool			drawOnMesh(const tamashii::DrawInfo* aDrawInfo) override;

					
	void			prepare(tamashii::RenderInfo_s& aRenderInfo) override;
	void			destroy() override;

					
	void			sceneLoad(tamashii::SceneBackendData aScene) override;
	void			sceneUnload(tamashii::SceneBackendData aScene) override;

					
	void			drawView(tamashii::ViewDef_s* aViewDef) override;
	void			drawUI(tamashii::UiConf_s* aUiConf) override;

	void			waitForNextFrame();
	void			startOptimizerThread(std::function<void(InteractiveAdjointLightTracing*, LightTraceOptimizer*, rvk::Buffer*, unsigned int, float, int)>);


	
	LightTraceOptimizer& getOptimizer() { return mLto; }
	void			runForward(const Eigen::Map<Eigen::VectorXd>&);
	double			runBackward(Eigen::VectorXd&);
	void			useCurrentRadianceAsTarget(bool clearWeights);
	OptimizerResult runOptimizer(LightTraceOptimizer::Optimizers, float, int);

	void			showTarget(const bool b) { mShowTarget = b; }
	void			clearGradVis();
	void			showGradVis(const std::optional<std::pair<tamashii::RefLight*, LightOptParams::PARAMS>>& p)
	{
		mGradImageSelection = p;
		mShowGrad = mGradVisAcc = mGradImageSelection.has_value();
	}
	void			showFDGradVis(const std::optional<std::pair<tamashii::RefLight*, LightOptParams::PARAMS>>& p, const float h)
	{
		mFDGradImageSelection = p;
		mFdGradH = h;
		mShowFDGrad = mGradVisAcc = mFDGradImageSelection.has_value();
	}

	std::unique_ptr<tamashii::Image>	getFrameImage();
	std::unique_ptr<tamashii::Image>	getGradImage();
private:
	tamashii::VulkanRenderRoot mRoot;

	
	struct VkData {
		explicit VkData(rvk::LogicalDevice& aDevice) :
			mGpuTd(&aDevice), mGpuMd(&aDevice), mGpuLd(&aDevice), mGpuBlas(&aDevice),
			mGlobalBuffer(&aDevice), mDescriptor(&aDevice), mShader(&aDevice), mPipelineCullNone(&aDevice),
			mPipelineCullFront(&aDevice), mPipelineCullBack(&aDevice), mShaderSHVis(&aDevice),
			mPipelineSHVisNone(&aDevice), mShaderSingleSHVis(&aDevice),
			mPipelineSingleSHVisNone(&aDevice), mDescriptorDrawOnMesh(&aDevice), mShaderDrawOnMesh(&aDevice),
			mPipelineDrawOnMesh(&aDevice), mCurrentPipeline(nullptr), mRadianceBufferCopy(&aDevice),
			mFdRadianceBufferCopy(&aDevice), mFd2RadianceBufferCopy(&aDevice),
			mDerivVisPipeline(&aDevice), mDerivVisShader(&aDevice),
			mDerivVisImageAccumulate(&aDevice), mDerivVisImageAccumulateCount(&aDevice) {}
		
		tamashii::TextureDataVulkan							mGpuTd;
		tamashii::MaterialDataVulkan						mGpuMd;
		tamashii::LightDataVulkan							mGpuLd;
		tamashii::GeometryDataBlasVulkan					mGpuBlas;

		rvk::Buffer											mGlobalBuffer;
		rvk::Descriptor										mDescriptor;
		rvk::RShader										mShader;
		rvk::RPipeline										mPipelineCullNone;
		rvk::RPipeline										mPipelineCullFront;
		rvk::RPipeline										mPipelineCullBack;
		rvk::RShader										mShaderSHVis;
		rvk::RPipeline										mPipelineSHVisNone;
		rvk::RShader										mShaderSingleSHVis;
		rvk::RPipeline										mPipelineSingleSHVisNone;
		rvk::Descriptor										mDescriptorDrawOnMesh;
		rvk::CShader										mShaderDrawOnMesh;
		rvk::CPipeline										mPipelineDrawOnMesh;
		rvk::RPipeline*										mCurrentPipeline;
		rvk::Buffer											mRadianceBufferCopy;
		rvk::Buffer											mFdRadianceBufferCopy;
		rvk::Buffer											mFd2RadianceBufferCopy;

		rvk::RTPipeline										mDerivVisPipeline;
		rvk::RTShader										mDerivVisShader;
		rvk::Image											mDerivVisImageAccumulate;
		rvk::Image											mDerivVisImageAccumulateCount;
	};
	struct VkFrameData {
		explicit VkFrameData(rvk::LogicalDevice& aDevice) : mGpuTlas(&aDevice), mColor(&aDevice), mDepth(&aDevice), mDerivVisDescriptor(&aDevice), mDerivVisImage(&aDevice) {}
		tamashii::GeometryDataTlasVulkan					mGpuTlas;
		rvk::Image											mColor;
		rvk::Image											mDepth;
		rvk::Descriptor										mDerivVisDescriptor;
		rvk::Image											mDerivVisImage;
	};

	std::optional<VkData>									mData;
	std::vector<VkFrameData>								mFrameData;
	std::deque<std::shared_ptr<tamashii::RefLight>>		*mLights;
	LightTraceOptimizer										mLto;

	bool													mSceneLoaded;
	bool													mShowTarget;
	bool													mShowAlpha;
	bool													mShowAdjointDeriv;
	float													mAdjointVisRange;
	bool													mAdjointVisLog;
	bool													mShowWireframeOverlay;
	glm::vec3												mWireframeColor;
	bool													mShowSH;
	bool													mShowInterpolatedSH;
	float													mShowSHSize;
	std::vector<std::string>								mCullMode;
	uint32_t												mActiveCullMode;
	bool													mShowGrad;
	float													mGradRange;
	bool													mGradVisLog;
	bool													mGradVisAcc;
	bool													mGradVisWeights;
	bool													mShowFDGrad;
	float													mFdGradH;
	bool													mDrawSetSH;
	
	static ccli::Var<int> mOptMaxIters;


	std::thread												mOptimizerThread;
	std::vector<std::string>								mOptimizerChoices;
	uint8_t													mOptimizerChoice;
	float													mOptimizerStepSize;

	std::optional<std::pair<tamashii::RefLight*, LightOptParams::PARAMS>> mGradImageSelection;
	std::optional<std::pair<tamashii::RefLight*, LightOptParams::PARAMS>> mFDGradImageSelection;

	std::condition_variable									mNextFrameCV;
};
