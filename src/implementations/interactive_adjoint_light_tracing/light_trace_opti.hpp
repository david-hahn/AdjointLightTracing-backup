#pragma once
#include <rvk/rvk.hpp>
#include <tamashii/renderer_vk/convenience/texture_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/geometry_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/material_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/light_to_gpu.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/renderer_vk/render_backend.hpp>

#include "parameter.hpp"

#include <Eigen/Dense>
#include <map>

constexpr char const* EXPORT_RADIANCE_ID = "radiance_data";
constexpr char const* EXPORT_RADIANCE_INFO_ID = "radiance_data_info";


class ObjectiveFunction;
class LightConstraint;
class LightOptParams;
class InteractiveAdjointLightTracing;

struct LBFGSppWrapperResult {
	double bestObjectiveValue;
	double lastPhi;
};

class LightTraceOptimizer {
public:
	enum Optimizers { LBFGS, GD, ADAM, FD_FORWARD, FD_CENTRAL, FD_CENTRAL_ADAM, CMA_ES };
	inline static const std::vector<std::string> optimizerNames = { "L-BFGS", "Grad. desc.", "ADAM", "FD-F-check", "FD-C-check", "FD-C ADAM", "CMA-ES (no grad.)" };
#ifdef IALT_USE_HARDCODED_TARGETS
	constexpr static bool useHardcodedTargets = true;
#else
	constexpr static bool useHardcodedTargets = false;
#endif
#ifdef IALT_USE_SPHERICAL_HARMONICS
	static uint32_t sphericalHarmonicOrder;
	static uint32_t entries_per_vertex;
	inline static const std::vector<std::string> shaderDefines = { "GLSL", "USE_SPHERICAL_HARMONICS" };
#else
	constexpr static uint32_t sphericalHarmonicOrder = 0;
	constexpr static uint64_t entries_per_vertex = 3; 
	inline static const std::vector<std::string> shaderDefines = { "GLSL" };
#endif
#ifdef IALT_USE_QUADRATIC_INTENSITY_OPT
	constexpr static bool useQuadraticIntensityOpt = true;
#else
	constexpr static bool useQuadraticIntensityOpt = false;
#endif
	struct vars{ 
		static ccli::Var<bool> objFuncOnGpu;
		static ccli::Var<bool,2> usePathTracing;
		static ccli::Var<uint32_t> numRaysXperLight;
		static ccli::Var<uint32_t> numRaysYperLight;
		static ccli::Var<uint32_t> numRaysPerTriangle;
		static ccli::Var<uint32_t> numSamples;
		static ccli::Var<bool> constRandSeed;
		static ccli::Var<std::string> runPredefinedTest;
		static ccli::Var<bool> useSHdiffOnlyCoeffObjective;
		static ccli::Var<bool> useSHdirOnlyCoeffObjective;
		static ccli::Var<float> useAABBconstraint;
		static ccli::Var<uint32_t> shOrder;
		static ccli::Var<bool> unphysicalNicePreview;
		static ccli::Var<float> useIntensityPenalty;

		static void initVars();
	};
					LightTraceOptimizer(const tamashii::VulkanRenderRoot& aRoot) :
						mSceneReady{ false }, mRoot{ aRoot }, mLights{ nullptr },
						mGpuTd{ nullptr }, mGpuMd{ nullptr }, mGpuLd{ nullptr }, mGpuBlas{ nullptr }, mGpuTlas{ nullptr },
						mAdjointDescriptor{ &aRoot.device }, mObjFuncDescriptor{ &aRoot.device }, mForwardShader{ &aRoot.device }, mForwardPipeline{ &aRoot.device },
						mForwardPTShader{ &aRoot.device }, mForwardPTPipeline{ &aRoot.device }, mBackwardShader{ &aRoot.device }, mBackwardPipeline{ &aRoot.device }, mBackwardPTShader{ &aRoot.device }, mBackwardPTPipeline{ &aRoot.device },
						mObjFuncShader{ &aRoot.device }, mObjFuncPipeline{ &aRoot.device }, mInfoBuffer{ &aRoot.device },
						mRadianceBuffer{ &aRoot.device }, mTargetRadianceBuffer{ &aRoot.device }, mTargetRadianceWeightsBuffer{ &aRoot.device },
						mVertexAreaBuffer{ &aRoot.device }, mVertexColorBuffer{ &aRoot.device },
						mLightDerivativesBuffer{ &aRoot.device }, mLightTextureDerivativesBuffer{ &aRoot.device }, mChannelWeightsBuffer{ &aRoot.device },
						mTriangleBuffer{ &aRoot.device }, mPhiBuffer{ &aRoot.device }, mCpuBuffer{ &aRoot.device }, mVertexCount{ 0 }, mTriangleCount{ 0 }, mBounces{ 2 },
						mFwdSimCount{ 0 }, mObjFcn{ nullptr }, mOptimizationRunning{ false }, mCurrentHistoryIndex{ -1 }, mForwardPT{ false }, mBackwardPT{ false } {}

					~LightTraceOptimizer() = default;

	void			init(tamashii::TextureDataVulkan* aGpuTd, tamashii::MaterialDataVulkan* aGpuMd, tamashii::LightDataVulkan* aGpuLd, tamashii::GeometryDataBlasVulkan* aGpuBlas, tamashii::GeometryDataTlasVulkan* aGpuTlas);
	void			sceneLoad(tamashii::SceneBackendData aScene, uint64_t aVertexCount);
	void			sceneUnload(tamashii::SceneBackendData aScene);
	void			destroy();

	void			importLightSettings();
	void			exportLightSettings();

	LBFGSppWrapperResult	optimize(uint32_t aOptimizer, rvk::Buffer* aRadianceBufferOut = nullptr, float aStepSize = 1.0, int aMaxIters = 200);
	void				optimizationRunning(bool aRun);
	bool				optimizationRunning();

	void			forward(Eigen::VectorXd& aParams, rvk::Buffer* aRadianceBufferOut = nullptr);
	double			backward(Eigen::VectorXd& aDerivParams);

					
	void			addCurrentStateToHistory(const Eigen::VectorXd& aParams);
	uint32_t		getHistorySize() const;
	int&			getCurrentHistoryIndex();
	void			selectParamsFromHistory(int aIndex);
	void			clearHistory();
	std::deque<Eigen::VectorXd> exportHistory();

					
	void			fillFiniteDiffRadianceBuffer(const tamashii::RefLight* aRefLight, LightOptParams::PARAMS aParam, float aH, rvk::Buffer* aFdRadianceBufferOut, rvk::Buffer* aFd2RadianceBufferOut);

	Eigen::VectorXd& getCurrentParams(){return mParams;}
	void			updateParamsFromScene(){
		if( mLights!=nullptr ){
			lightsToParameterVector(mParams);
			lightTextureToParameterVector(mParams);
		}
	}
	int&			bounces(){return mBounces;}
	void			setTargetWeights(float aAlpha);
	void			buildObjectiveFunction(tamashii::SceneBackendData aScene);
	void			copyRadianceToTarget() const;
	void			copyTargetToMesh(const tamashii::SceneBackendData& aScene) const;
	void			copyMeshToTarget(const tamashii::SceneBackendData& aScene) const;
	void			setTargetRadianceBufferForScene(std::optional<glm::vec3>, std::optional<float>) const;
	void			setTargetRadianceBufferForMesh(const tamashii::RefMesh*, std::optional<glm::vec3>, std::optional<float>) const;
	rvk::Buffer*	getTargetRadianceBuffer();
	rvk::Buffer*	getTargetRadianceWeightsBuffer();
	rvk::Buffer*	getChannelWeightsBuffer();
	rvk::Descriptor*getAdjointDescriptor();
	LightOptParams& optimizationParametersByLightRef(const tamashii::Ref&);

	rvk::Buffer*	getVtxTextureColorBuffer();
	rvk::Buffer*	getVtxAreaBuffer();
	std::map<tamashii::Ref*, LightOptParams>& getLightOptParams();

	uint32_t		getActiveParameterCount() const { return LightOptParams::getActiveParameterCount(mLightParams); }

    void			runPredefinedTestCase(InteractiveAdjointLightTracing* ialt, const tamashii::SceneBackendData& aScene, const std::string& aCaseName, rvk::Buffer* aRadianceBufferOut = nullptr);

	void			useForwardPT(const bool b) { mForwardPT = b; }
	bool			useForwardPT() { return mForwardPT; }

	void			useBackwardPT(const bool b) { mBackwardPT = b; }
	bool			useBackwardPT() { return mBackwardPT; }

	void			lightsToParameterVector(Eigen::VectorXd& aParams);
	void			parameterVectorToLights(Eigen::VectorXd& aParams);

private:
	void			updateLightParamsIfNecessary();
	void			sceneMeshToEigenArrays(const tamashii::SceneBackendData& aScene);
	void			writeLegacyVTKpointData(Eigen::MatrixXi& aElems, Eigen::MatrixXf& aCoords, const rvk::Buffer* aDataBuffer,
	                                        const std::string& aFilename, const std::string& aDataname);
	void			lightDerivativesToVector(Eigen::VectorXd& aDerivParams);

	void			lightTextureToParameterVector(Eigen::VectorXd& aParams);
	void			parameterVectorToLightTexture(Eigen::VectorXd& aParams);
	double			lightTextureDerivativesToVector(Eigen::VectorXd& aDerivParams);
	tamashii::Texture* mFirstEmissiveTexture; 

	bool											mSceneReady;
	tamashii::VulkanRenderRoot						mRoot;

	std::deque<std::shared_ptr<tamashii::RefModel>>* mModels;
	std::deque<std::shared_ptr<tamashii::RefLight>>* mLights;
	std::deque<tamashii::Image*>*	 mImages;
	std::deque<tamashii::Texture*>*	 mTextures;
	std::deque<tamashii::Material*>* mMaterials;

	std::map<tamashii::Ref*, LightOptParams>		mLightParams;
	std::deque<LightConstraint*>					mConstraints;

	tamashii::TextureDataVulkan*					mGpuTd;
	tamashii::MaterialDataVulkan*					mGpuMd;
	tamashii::LightDataVulkan*						mGpuLd;
	tamashii::GeometryDataBlasVulkan*				mGpuBlas;
	tamashii::GeometryDataTlasVulkan*				mGpuTlas;

	rvk::Descriptor									mAdjointDescriptor;
	rvk::Descriptor									mObjFuncDescriptor;
	rvk::RTShader									mForwardShader;
	rvk::RTPipeline									mForwardPipeline;
	rvk::RTShader									mForwardPTShader;
	rvk::RTPipeline									mForwardPTPipeline;
	rvk::RTShader									mBackwardShader;
	rvk::RTPipeline									mBackwardPipeline;
	rvk::RTShader									mBackwardPTShader;
	rvk::RTPipeline									mBackwardPTPipeline;
	rvk::CShader									mObjFuncShader;
	rvk::CPipeline									mObjFuncPipeline;

	rvk::Buffer										mInfoBuffer;
	rvk::Buffer										mRadianceBuffer;
	rvk::Buffer										mTargetRadianceBuffer;
	rvk::Buffer										mTargetRadianceWeightsBuffer;
	rvk::Buffer										mVertexAreaBuffer;
	rvk::Buffer										mVertexColorBuffer;
	rvk::Buffer										mLightDerivativesBuffer;
	rvk::Buffer										mLightTextureDerivativesBuffer;
	rvk::Buffer										mChannelWeightsBuffer;
	rvk::Buffer										mTriangleBuffer;
	rvk::Buffer										mPhiBuffer;
	rvk::Buffer										mCpuBuffer;

	uint64_t										mVertexCount;
	uint64_t										mTriangleCount;
	int												mBounces;
	uint32_t										mFwdSimCount; 
	
    
    Eigen::MatrixXf									mCoords;
	Eigen::MatrixXf									mVertexNormalTangent;
	Eigen::MatrixXi									mElems;
	Eigen::VectorXf									mVtxArea;

	
	Eigen::SparseMatrix<double>						mAplusAlphaL;
	Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>, Eigen::Upper | Eigen::Lower> mSmoothingSolver;
	
	
	

	Eigen::VectorXd									mParams; 

	ObjectiveFunction*								mObjFcn;

	std::mutex										mOptimizationMutex;
	bool											mOptimizationRunning;
	std::deque<Eigen::VectorXd>						mHistory;
	int												mCurrentHistoryIndex;

	bool											mForwardPT;
	bool											mBackwardPT;
	
	uint32_t										mForwardTimeCount = 0;
	uint32_t										mBackwardTimeCount = 0;
	uint32_t										mForwardTimeSum = 0;
	uint32_t										mBackwardTimeSum = 0;
};
