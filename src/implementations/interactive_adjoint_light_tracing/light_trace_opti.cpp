#include "light_trace_opti.hpp"
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/scene/ref_entities.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/model.hpp>
#include "LBFGSppWrapper.hpp"

#include "../../../assets/shader/ialt/defines.h"


#include <tamashii/core/platform/system.hpp>
#include <tamashii/core/platform/filewatcher.hpp>

#include <Eigen/Eigen>
#include "objectivefunction.hpp"
#include "constraint.hpp"

#include <sstream>
#include <fstream>

#include "ialt.hpp"

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif

T_USE_NAMESPACE

ccli::Var<bool>			LightTraceOptimizer::vars::objFuncOnGpu("", "objFuncOnGpu", true, ccli::Flag::ConfigRead, "Calculate objective function on gpu.");
ccli::Var<bool, 2>		LightTraceOptimizer::vars::usePathTracing("", "usePathTracing", { false, false }, ccli::Flag::ConfigRead, "Use path tracing (forward,backward) instead of light tracing (for comparisons)", [](ccli::Var<bool, 2>::TStorage::TParameter v)
{
		if (!Common::getInstance().getRenderSystem()->hasBackend()) return;
		auto* currentImplementation = Common::getInstance().getRenderSystem()->getCurrentBackendImplementations();
		if (!currentImplementation) return;
		InteractiveAdjointLightTracing* ialt = dynamic_cast<InteractiveAdjointLightTracing*>(currentImplementation);
		if (ialt) {
			ialt->getOptimizer().useForwardPT(v[0]);
			ialt->getOptimizer().useBackwardPT(v[1]);
		}
});
ccli::Var<uint32_t>		LightTraceOptimizer::vars::numRaysXperLight("", "numRaysXperLight", 2048, ccli::Flag::ConfigRead, "Number of rays per light source X.");
ccli::Var<uint32_t>		LightTraceOptimizer::vars::numRaysYperLight("", "numRaysYperLight", 2048, ccli::Flag::ConfigRead, "Number of rays per light source Y.");
ccli::Var<uint32_t>		LightTraceOptimizer::vars::numRaysPerTriangle("", "numRaysPerTriangle", 128, ccli::Flag::ConfigRead, "Number of rays per triangle.");
ccli::Var<uint32_t>		LightTraceOptimizer::vars::numSamples("", "numSamples", 2, ccli::Flag::ConfigRead, "Number of samples.");
ccli::Var<bool>			LightTraceOptimizer::vars::constRandSeed("", "constRandSeed", false, ccli::Flag::ConfigRead, "Use constant random seed for each simulation (default off).");
ccli::Var<std::string>	LightTraceOptimizer::vars::runPredefinedTest("", "runPredefinedTest", /*empty by default*/"", ccli::Flag::ConfigRead, "Run a predefined test case.");
ccli::Var<bool>			LightTraceOptimizer::vars::useSHdiffOnlyCoeffObjective("", "useSHdiffOnlyCoeffObjective", false, ccli::Flag::ConfigRead, "Use all SH coefficients when evaluating objectives (i.e. directionally-dependent target; default off).");
ccli::Var<bool>			LightTraceOptimizer::vars::useSHdirOnlyCoeffObjective("", "useSHdirOnlyCoeffObjective", false, ccli::Flag::ConfigRead, "Use all SH coefficients when evaluating objectives (i.e. directionally-dependent target; default off).");
ccli::Var<float>		LightTraceOptimizer::vars::useAABBconstraint("", "useAABBconstraint", -1.0f, ccli::Flag::ConfigRead, "Constrain the position of all lights to the scene's AABB using the specified penalty factor (default < 0.0 ==> off ).");
ccli::Var<uint32_t>		LightTraceOptimizer::vars::shOrder("", "shOrder", 5, ccli::Flag::ConfigRead, "Order of spherical harmonic space (will result in 3*(order+1)^2 coefficients per vertex");
ccli::Var<bool>			LightTraceOptimizer::vars::unphysicalNicePreview("","unphysicalNicePreview", false, ccli::Flag::ConfigRead, "Use unphysical but nice looking preview (default off).");
ccli::Var<float>		LightTraceOptimizer::vars::useIntensityPenalty("", "useIntensityPenalty", -1.0f, ccli::Flag::ConfigRead, "Penalize intensities of lights to encourage energy-efficient solutions using the specified penalty factor (default < 0.0 ==> off ).");

void LightTraceOptimizer::vars::initVars() {
	tamashii::var::default_implementation.value("ialt");
	tamashii::var::unique_model_refs.value(true);
	tamashii::var::unique_model_refs.lock();
	tamashii::var::unique_light_refs.value(true);
	tamashii::var::unique_light_refs.lock();
}


#ifdef IALT_USE_SPHERICAL_HARMONICS
	uint32_t LightTraceOptimizer::sphericalHarmonicOrder; 
	uint32_t LightTraceOptimizer::entries_per_vertex;
#endif

constexpr float SQRT4PI = 3.54490770181103f;

namespace {
	ConeAngleTanhParameterization coneAngleParameterization;

	void TestOfficeTargetHardcoded(Eigen::SparseMatrix<float>& aA, Eigen::Ref<Eigen::MatrixXf> aRadianceTarget, const tamashii::SceneBackendData& aScene, Eigen::Ref<Eigen::VectorXf> aVtxArea, const uint32_t aEntriesPerVertex){
#ifdef IALT_USE_SPHERICAL_HARMONICS
		spdlog::warn("Using a hardcoded target intended for diffuse-only calculation with spherical harmonics - this may result in unwanted behaviour");
#endif
		Eigen::VectorXf weights;
		weights.resize(aVtxArea.size());
		if( aRadianceTarget.rows() != aVtxArea.size() || aRadianceTarget.cols() != aEntriesPerVertex ){
			spdlog::error("TwoPlanesLowResTargetHardcoded - size mismatch");
			return;
		}
		aRadianceTarget.setZero();
		weights.setZero(); 

		size_t vertexOffset = 0;
		for (const auto refModel : aScene.refModels) {
			for (const auto& refMesh : refModel->refMeshes) {
				if (refMesh->mesh->getMaterial()->getName().find("matTableTop") != std::string::npos) {
					
					for (size_t i = 0; i < refMesh->mesh->getVertexCount(); ++i) {
						aRadianceTarget.row(i + vertexOffset).setConstant(0.7f); 
						weights[i + vertexOffset] = aVtxArea[i + vertexOffset];
					}
				}
				vertexOffset += refMesh->mesh->getVertexCount();
			}
		}
		aA = weights.asDiagonal();
		
	}
	void TwoPlanesLowResTargetHardcoded(Eigen::SparseMatrix<float>& aA, Eigen::Ref<Eigen::MatrixXf> aRadianceTarget, tamashii::SceneBackendData& aScene, const Eigen::Ref<Eigen::VectorXf>& aVtxArea, const uint32_t aEntriesPerVertex){
#ifdef IALT_USE_SPHERICAL_HARMONICS
		spdlog::warn("Using a hardcoded target intended for diffuse-only calculation with spherical harmonics - this may result in unwanted behaviour");
#endif
		if( aRadianceTarget.rows() != aVtxArea.size() || aRadianceTarget.cols() != aEntriesPerVertex ){
			spdlog::error("TwoPlanesLowResTargetHardcoded - size mismatch");
			return;
		}

		aA = aVtxArea.asDiagonal();

		Eigen::Matrix<float,-1,-1,Eigen::RowMajor> targetData; targetData.resize(aRadianceTarget.cols(), aRadianceTarget.rows()); targetData.setZero();
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		targetData << 
			7.57474e-06f, 7.90775e-06f, 8.70094e-06f, 1.47524e-05f, 1.52173e-05f, 1.7494e-05f, 2.12507e-05f, 2.46218e-05f, 2.59408e-05f, 9.69909e-06f, 1.07638e-05f, 1.10725e-05f, 1.38656e-05f, 1.65031e-05f, 1.72848e-05f, 1.34196e-05f, 1.71885e-05f, 1.66427e-05f, 9.30682e-06f, 1.10923e-05f, 1.07719e-05f, 3.10686e-05f, 3.39174e-05f, 3.62256e-05f, 3.02396e-05f, 3.64864e-05f, 3.49482e-05f, 2.06291e-05f, 2.6138e-05f, 2.51544e-05f, 1.98381e-05f, 1.94349e-05f, 2.21274e-05f, 2.64558e-05f, 2.63055e-05f, 3.03335e-05f, 1.88168e-05f, 1.98834e-05f, 2.25827e-05f, 1.0419e-05f, 1.11579e-05f, 1.23782e-05f, 1.25427e-05f, 1.39349e-05f, 1.53494e-05f, 8.88831e-06f, 9.57268e-06f, 1.01802e-05f, 1.15738e-05f, 1.33709e-05f, 9.41955e-06f, 1.69118e-05f, 2.02781e-05f, 1.29483e-05f, 2.3186e-05f, 2.9145e-05f, 2.87218e-05f, 3.36373e-05f, 3.70454e-05f, 2.07535e-05f, 2.42053e-05f, 3.13213e-05f, 2.47705e-05f, 2.93132e-05f, 2.01656e-05f, 3.94988e-05f, 3.72771e-05f, 3.31998e-05f, 3.46041e-05f, 3.93605e-05f, 1.38786e-05f, 1.07834e-05f, 1.34724e-05f, 9.2437e-06f, 2.11742e-05f, 1.63591e-05f, 9.58788e-06f, 1.73335e-05f, 9.3366e-06f, 8.28966e-06f, 7.01666e-06f, 1.85549e-07f, 1.88524e-07f, 1.97297e-07f, 2.55626e-07f, 2.36577e-07f, 2.77261e-07f, 0.000590195f, 0.00492815f, 0.0038846f, 3.25929e-07f, 3.14665e-07f, 3.53224e-07f, 4.10825e-07f, 4.01237e-07f, 4.85705e-07f, 5.41412e-07f, 5.57943e-07f, 6.36359e-07f, 3.80519e-07f, 4.03081e-07f, 4.31759e-07f, 0.0047644f, 0.00424187f, 0.00365534f, 0.000989476f, 0.00193948f, 0.000276268f, 3.65918e-05f, 0.00218736f, 0.000583217f, 2.86903e-07f, 2.51972e-07f, 2.83693e-07f, 0.00512729f, 0.000123156f, 0.00250774f, 5.18057e-07f, 4.66286e-07f, 0.0017125f, 2.19911e-07f, 2.09308e-07f, 2.30863e-07f, 3.21058e-07f, 2.86424e-07f, 3.46216e-07f, 2.5809e-07f, 2.39648e-07f, 2.70013e-07f, 2.56008e-07f, 3.53845e-07f, 2.87457e-07f, 3.10831e-07f, 0.00028018f, 2.16448e-07f, 0.000219513f, 0.00647424f, 3.46037e-07f, 0.000204144f, 0.000891894f, 2.46429e-07f, 2.98464e-07f, 0.00321003f, 3.50822e-05f, 5.24205e-05f, 8.09144e-07f, 0.000153792f, 9.14327e-07f, 9.8858e-07f, 9.7355e-07f, 0.000758168f, 4.75244e-07f, 4.83227e-07f, 5.94241e-07f, 4.20177e-07f, 0.000294144f, 6.91245e-07f, 3.39433e-07f, 2.58298e-07f, 1.94677e-07f, 2.19634e-07f, 1.75759e-07,
			7.57474e-06f, 7.90775e-06f, 8.70094e-06f, 1.47524e-05f, 1.52173e-05f, 1.7494e-05f, 2.12507e-05f, 2.46218e-05f, 2.59408e-05f, 9.69909e-06f, 1.07638e-05f, 1.10725e-05f, 1.38656e-05f, 1.65031e-05f, 1.72848e-05f, 1.34196e-05f, 1.71885e-05f, 1.66427e-05f, 9.30682e-06f, 1.10923e-05f, 1.07719e-05f, 3.10686e-05f, 3.39174e-05f, 3.62256e-05f, 3.02396e-05f, 3.64864e-05f, 3.49482e-05f, 2.06291e-05f, 2.6138e-05f, 2.51544e-05f, 1.98381e-05f, 1.94349e-05f, 2.21274e-05f, 2.64558e-05f, 2.63055e-05f, 3.03335e-05f, 1.88168e-05f, 1.98833e-05f, 2.25827e-05f, 1.0419e-05f, 1.11579e-05f, 1.23782e-05f, 1.25427e-05f, 1.39349e-05f, 1.53494e-05f, 8.88831e-06f, 9.57268e-06f, 1.01802e-05f, 1.15738e-05f, 1.33709e-05f, 9.41956e-06f, 1.69118e-05f, 2.02781e-05f, 1.29483e-05f, 2.3186e-05f, 2.9145e-05f, 2.87218e-05f, 3.36373e-05f, 3.70454e-05f, 2.07535e-05f, 2.42053e-05f, 3.13213e-05f, 2.47705e-05f, 2.93132e-05f, 2.01656e-05f, 3.94988e-05f, 3.72771e-05f, 3.31998e-05f, 3.46041e-05f, 3.93605e-05f, 1.38786e-05f, 1.07834e-05f, 1.34724e-05f, 9.2437e-06f, 2.11742e-05f, 1.63591e-05f, 9.58788e-06f, 1.73335e-05f, 9.3366e-06f, 8.28966e-06f, 7.01666e-06f, 1.85549e-07f, 1.88524e-07f, 1.97297e-07f, 2.55626e-07f, 2.36577e-07f, 2.77261e-07f, 0.000590195f, 0.00492814f, 0.0038846f, 3.25929e-07f, 3.14665e-07f, 3.53224e-07f, 4.10825e-07f, 4.01237e-07f, 4.85705e-07f, 5.41412e-07f, 5.57943e-07f, 6.36359e-07f, 3.80519e-07f, 4.03081e-07f, 4.31759e-07f, 0.00476439f, 0.00424186f, 0.00365534f, 0.000989476f, 0.00193948f, 0.000276268f, 3.65918e-05f, 0.00218736f, 0.000583217f, 2.86903e-07f, 2.51972e-07f, 2.83693e-07f, 0.00512729f, 0.000123156f, 0.00250774f, 5.18057e-07f, 4.66286e-07f, 0.00171249f, 2.19911e-07f, 2.09308e-07f, 2.30863e-07f, 3.21058e-07f, 2.86424e-07f, 3.46216e-07f, 2.5809e-07f, 2.39648e-07f, 2.70013e-07f, 2.56008e-07f, 3.53845e-07f, 2.87457e-07f, 3.10831e-07f, 0.00028018f, 2.16448e-07f, 0.000219513f, 0.00647425f, 3.46037e-07f, 0.000204144f, 0.000891894f, 2.46429e-07f, 2.98464e-07f, 0.00321004f, 3.50822e-05f, 5.24205e-05f, 8.09144e-07f, 0.000153792f, 9.14327e-07f, 9.8858e-07f, 9.7355e-07f, 0.000758168f, 4.75244e-07f, 4.83227e-07f, 5.94241e-07f, 4.20177e-07f, 0.000294144f, 6.91245e-07f, 3.39433e-07f, 2.58298e-07f, 1.94677e-07f, 2.19634e-07f, 1.75759e-07,
			7.57474e-06f, 7.90775e-06f, 8.70094e-06f, 1.47524e-05f, 1.52173e-05f, 1.7494e-05f, 2.12507e-05f, 2.46218e-05f, 2.59408e-05f, 9.69909e-06f, 1.07638e-05f, 1.10725e-05f, 1.38656e-05f, 1.65031e-05f, 1.72848e-05f, 1.34196e-05f, 1.71885e-05f, 1.66427e-05f, 9.30682e-06f, 1.10923e-05f, 1.07719e-05f, 3.10686e-05f, 3.39174e-05f, 3.62256e-05f, 3.02396e-05f, 3.64864e-05f, 3.49481e-05f, 2.06291e-05f, 2.6138e-05f, 2.51544e-05f, 1.98381e-05f, 1.94349e-05f, 2.21274e-05f, 2.64558e-05f, 2.63055e-05f, 3.03335e-05f, 1.88168e-05f, 1.98833e-05f, 2.25827e-05f, 1.0419e-05f, 1.11579e-05f, 1.23782e-05f, 1.25427e-05f, 1.39349e-05f, 1.53494e-05f, 8.88831e-06f, 9.57268e-06f, 1.01802e-05f, 1.15738e-05f, 1.33709e-05f, 9.41955e-06f, 1.69118e-05f, 2.02781e-05f, 1.29483e-05f, 2.3186e-05f, 2.9145e-05f, 2.87218e-05f, 3.36373e-05f, 3.70454e-05f, 2.07535e-05f, 2.42053e-05f, 3.13213e-05f, 2.47705e-05f, 2.93132e-05f, 2.01656e-05f, 3.94988e-05f, 3.72771e-05f, 3.31998e-05f, 3.46041e-05f, 3.93605e-05f, 1.38786e-05f, 1.07834e-05f, 1.34724e-05f, 9.2437e-06f, 2.11742e-05f, 1.63591e-05f, 9.58788e-06f, 1.73335e-05f, 9.3366e-06f, 8.28966e-06f, 7.01666e-06f, 1.85549e-07f, 1.88524e-07f, 1.97297e-07f, 2.55626e-07f, 2.36577e-07f, 2.77261e-07f, 0.000590195f, 0.00492815f, 0.0038846f, 3.25929e-07f, 3.14665e-07f, 3.53224e-07f, 4.10825e-07f, 4.01237e-07f, 4.85705e-07f, 5.41412e-07f, 5.57943e-07f, 6.36359e-07f, 3.80519e-07f, 4.03081e-07f, 4.31759e-07f, 0.00476439f, 0.00424186f, 0.00365534f, 0.000989475f, 0.00193948f, 0.000276268f, 3.65918e-05f, 0.00218736f, 0.000583217f, 2.86903e-07f, 2.51972e-07f, 2.83693e-07f, 0.00512729f, 0.000123156f, 0.00250774f, 5.18057e-07f, 4.66286e-07f, 0.00171249f, 2.19911e-07f, 2.09308e-07f, 2.30863e-07f, 3.21058e-07f, 2.86424e-07f, 3.46216e-07f, 2.5809e-07f, 2.39648e-07f, 2.70013e-07f, 2.56008e-07f, 3.53845e-07f, 2.87457e-07f, 3.10831e-07f, 0.00028018f, 2.16448e-07f, 0.000219513f, 0.00647425f, 3.46037e-07f, 0.000204144f, 0.000891894f, 2.46429e-07f, 2.98464e-07f, 0.00321003f, 3.50822e-05f, 5.24206e-05f, 8.09144e-07f, 0.000153792f, 9.14327e-07f, 9.8858e-07f, 9.7355e-07f, 0.000758167f, 4.75244e-07f, 4.83227e-07f, 5.94241e-07f, 4.20177e-07f, 0.000294144f, 6.91245e-07f, 3.39433e-07f, 2.58298e-07f, 1.94677e-07f, 2.19634e-07f, 1.75759e-07f;
		aRadianceTarget = targetData.transpose();
		
	}

	double vectorCotan(const Eigen::Vector3f& aA, const Eigen::Vector3f& aB){ 
		const double ab = aA.dot(aB), aa = aA.squaredNorm(), bb = aB.squaredNorm();
		return sqrt(ab * ab / (aa * bb - ab * ab)) * (ab >= 0 ? 1.0 : -1.0);
	}
	void cotanLaplacian(Eigen::SparseMatrix<double>& aLap, const Eigen::MatrixXf& aCoords, const Eigen::MatrixXi& aElems){
		std::vector<Eigen::Triplet<double>> lapTriplets;
		lapTriplets.reserve(9 * aElems.rows());

		for (uint32_t k = 0; k < aElems.rows(); ++k) {
			const int32_t adof = aElems(k, 0), bdof = aElems(k, 1), cdof = aElems(k, 2);
			Eigen::Vector3f a(aCoords.row(adof)), b(aCoords.row(bdof)), c(aCoords.row(cdof));

			const double cot1 = vectorCotan(a - c, b - c);
			const double cot2 = vectorCotan(b - a, c - a);
			const double cot3 = vectorCotan(c - b, a - b);

			
			lapTriplets.emplace_back(Eigen::Triplet(adof, adof, 0.5 * (cot1 + cot3)));
			lapTriplets.emplace_back(Eigen::Triplet(adof, bdof, 0.5 * (-cot1)));
			lapTriplets.emplace_back(Eigen::Triplet(adof, cdof, 0.5 * (-cot3)));

			
			lapTriplets.emplace_back(Eigen::Triplet(bdof, adof, 0.5 * (-cot1)));
			lapTriplets.emplace_back(Eigen::Triplet(bdof, bdof, 0.5 * (cot1 + cot2)));
			lapTriplets.emplace_back(Eigen::Triplet(bdof, cdof, 0.5 * (-cot2)));

			
			lapTriplets.emplace_back(Eigen::Triplet(cdof, adof, 0.5 * (-cot3)));
			lapTriplets.emplace_back(Eigen::Triplet(cdof, bdof, 0.5 * (-cot2)));
			lapTriplets.emplace_back(Eigen::Triplet(cdof, cdof, 0.5 * (cot2 + cot3)));
		}

		aLap.resize(aCoords.rows(), aCoords.rows());
		aLap.setFromTriplets(lapTriplets.begin(), lapTriplets.end());
	}
}

void LightTraceOptimizer::init(tamashii::TextureDataVulkan* aGpuTd, tamashii::MaterialDataVulkan* aGpuMd, tamashii::LightDataVulkan* aGpuLd, tamashii::GeometryDataBlasVulkan* aGpuBlas, tamashii::GeometryDataTlasVulkan* aGpuTlas)
{
	mGpuTd = aGpuTd;
	mGpuMd = aGpuMd;
	mGpuLd = aGpuLd;
	mGpuBlas = aGpuBlas;
	mGpuTlas = aGpuTlas;

	mAdjointDescriptor.reserve(8);
	mAdjointDescriptor.addAccelerationStructureKHR(ADJOINT_DESC_TLAS_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_INFO_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_INDEX_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_VERTEX_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_MATERIAL_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_GEOMETRY_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_LIGHT_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_AREA_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_LIGHT_DERIVATIVES_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_LIGHT_TEXTURE_DERIVATIVES_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.addStorageBuffer(ADJOINT_DESC_TRIANGLE_BUFFER_BINDING, rvk::Shader::Stage::RAYGEN);
	mAdjointDescriptor.finish(false);

	mObjFuncDescriptor.reserve(3);
	mObjFuncDescriptor.addStorageBuffer(OBJ_DESC_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mObjFuncDescriptor.addStorageBuffer(OBJ_DESC_TARGET_RADIANCE_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mObjFuncDescriptor.addStorageBuffer(OBJ_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mObjFuncDescriptor.addStorageBuffer(OBJ_DESC_AREA_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mObjFuncDescriptor.addStorageBuffer(OBJ_DESC_CHANNEL_WEIGHTS_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mObjFuncDescriptor.addStorageBuffer(OBJ_DESC_PHI_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mObjFuncDescriptor.addStorageBuffer(OBJ_DESC_VERTEX_COLOR_BUFFER_BINDING, rvk::Shader::Stage::COMPUTE);
	mObjFuncDescriptor.finish(false);

	uint32_t constData[3] = { sphericalHarmonicOrder, entries_per_vertex, (uint32_t)(LightTraceOptimizer::vars::unphysicalNicePreview.asBool().value()) };

	mForwardShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, IALT_SHADER_DIR "forward_rgen.glsl", shaderDefines);		
	mForwardShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::CLOSEST_HIT, IALT_SHADER_DIR "forward_rchit.glsl", shaderDefines);	
	mForwardShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::MISS, IALT_SHADER_DIR "forward_rmiss.glsl", shaderDefines);			
	mForwardShader.addGeneralShaderGroup(0);
	mForwardShader.addHitShaderGroup(1);
	mForwardShader.addGeneralShaderGroup(2);
	mForwardShader.addConstant(0, 0, 4u, 0u);
	mForwardShader.addConstant(0, 1, 4u, 4u);
	mForwardShader.addConstant(0, 2, 4u, 8u);
	mForwardShader.setConstantData(0, constData, 12u);
	mForwardShader.finish();

	mForwardPTShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, IALT_SHADER_DIR "forward_pt_rgen.glsl", shaderDefines);		
	mForwardPTShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::CLOSEST_HIT, IALT_SHADER_DIR "forward_rchit.glsl", shaderDefines);	
	mForwardPTShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::MISS, IALT_SHADER_DIR "forward_rmiss.glsl", shaderDefines);			
	mForwardPTShader.addGeneralShaderGroup(0);
	mForwardPTShader.addHitShaderGroup(1);
	mForwardPTShader.addGeneralShaderGroup(2);
	mForwardPTShader.addConstant(0, 0, 4u, 0u);
	mForwardPTShader.addConstant(0, 1, 4u, 4u);
	mForwardPTShader.addConstant(0, 2, 4u, 8u);
	mForwardPTShader.setConstantData(0, constData, 12u);
	mForwardPTShader.finish();

	mBackwardShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, IALT_SHADER_DIR "backward_rgen.glsl", shaderDefines);		
	mBackwardShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::CLOSEST_HIT, IALT_SHADER_DIR "forward_rchit.glsl", shaderDefines);	
	mBackwardShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::MISS, IALT_SHADER_DIR "forward_rmiss.glsl", shaderDefines);		
	mBackwardShader.addGeneralShaderGroup(0);
	mBackwardShader.addHitShaderGroup(1);
	mBackwardShader.addGeneralShaderGroup(2);
	mBackwardShader.addConstant(0, 0, 4u, 0u);
	mBackwardShader.addConstant(0, 1, 4u, 4u);
	mBackwardShader.addConstant(0, 2, 4u, 8u);
	mBackwardShader.setConstantData(0, constData, 12u);
	mBackwardShader.finish();

	mBackwardPTShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::RAYGEN, IALT_SHADER_DIR "backward_pt_rgen.glsl", shaderDefines);		
	mBackwardPTShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::CLOSEST_HIT, IALT_SHADER_DIR "forward_rchit.glsl", shaderDefines);	
	mBackwardPTShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::MISS, IALT_SHADER_DIR "forward_rmiss.glsl", shaderDefines);		
	mBackwardPTShader.addGeneralShaderGroup(0);
	mBackwardPTShader.addHitShaderGroup(1);
	mBackwardPTShader.addGeneralShaderGroup(2);
	mBackwardPTShader.addConstant(0, 0, 4u, 0u);
	mBackwardPTShader.addConstant(0, 1, 4u, 4u);
	mBackwardPTShader.addConstant(0, 2, 4u, 8u);
	mBackwardPTShader.setConstantData(0, constData, 12u);
	mBackwardPTShader.finish();

	tamashii::FileWatcher::getInstance().watchFile(IALT_SHADER_DIR "forward_rgen.glsl", [this]() { mForwardShader.reloadShader(0); });
	
	mForwardPipeline.setShader(&mForwardShader);
	mForwardPipeline.addDescriptorSet({aGpuTd->getDescriptor(), &mAdjointDescriptor});
	mForwardPipeline.finish();

	mForwardPTPipeline.setShader(&mForwardPTShader);
	mForwardPTPipeline.addDescriptorSet({ aGpuTd->getDescriptor(), &mAdjointDescriptor });
	mForwardPTPipeline.finish();

	mBackwardPipeline.setShader(&mBackwardShader);
	mBackwardPipeline.addDescriptorSet({aGpuTd->getDescriptor(), &mAdjointDescriptor});
	mBackwardPipeline.finish();

	mBackwardPTPipeline.setShader(&mBackwardPTShader);
	mBackwardPTPipeline.addDescriptorSet({ aGpuTd->getDescriptor(), &mAdjointDescriptor });
	mBackwardPTPipeline.finish();

	mObjFuncShader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::COMPUTE, IALT_SHADER_DIR "obj_func.comp", shaderDefines);
	mObjFuncShader.addConstant(0, 0, 4u, 0u);
	mObjFuncShader.addConstant(0, 1, 4u, 4u);
	mObjFuncShader.addConstant(0, 2, 4u, 8u);
	mObjFuncShader.setConstantData(0, constData, 12u);
	mObjFuncShader.finish();
	mObjFuncPipeline.setShader(&mObjFuncShader);
	mObjFuncPipeline.addDescriptorSet({ &mObjFuncDescriptor });
	mObjFuncPipeline.addPushConstant(rvk::Shader::Stage::COMPUTE, 0, sizeof(uint32_t));
	mObjFuncPipeline.finish();

	mChannelWeightsBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD, entries_per_vertex * sizeof(float), rvk::Buffer::Location::DEVICE);
	mPhiBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::DOWNLOAD, sizeof(double), rvk::Buffer::Location::DEVICE);
	mCpuBuffer.create(rvk::Buffer::Use::UPLOAD | rvk::Buffer::Use::DOWNLOAD, std::max(sizeof(double), sizeof(AdjointInfo_s)), rvk::Buffer::Location::HOST_COHERENT);
	mCpuBuffer.mapBuffer();

	if (LightTraceOptimizer::vars::usePathTracing[0]) useForwardPT(true);
	if (LightTraceOptimizer::vars::usePathTracing[1]) useBackwardPT(true);
}

void LightTraceOptimizer::setTargetWeights(const float aAlpha)
{
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	uint32_t floatBits = 0;
	std::memcpy(&floatBits, &aAlpha, sizeof(float));
	stc.begin();
	mTargetRadianceWeightsBuffer.CMD_FillBuffer(stc.buffer(), floatBits);
	stc.end();
}

void LightTraceOptimizer::buildObjectiveFunction(tamashii::SceneBackendData aScene) {
	
	for (const LightConstraint* lc : mConstraints) delete lc;
	mConstraints.clear();

	
	if (vars::useAABBconstraint > 0.0f) {
		spdlog::info("Lights in AABB constraint active with penalty factor {}", vars::useAABBconstraint.value());
		mConstraints.push_back(new LightsInAABBConstraint(
			mCoords.col(0).minCoeff(), mCoords.col(0).maxCoeff(),  
			mCoords.col(1).minCoeff(), mCoords.col(1).maxCoeff(),
			mCoords.col(2).minCoeff(), mCoords.col(2).maxCoeff()
		));
		for (auto rl : *mLights) {
			mConstraints.back()->addLight(rl.get());
		}
		mConstraints.back()->setPenaltyFactor(vars::useAABBconstraint.value());
	}

	
	if (vars::useIntensityPenalty > 0.0f) {
		spdlog::info("Light intensity penalty active with factor {}", vars::useIntensityPenalty.value());
		mConstraints.push_back(new LightsIntensityPenalty(vars::useIntensityPenalty.value()));
		for (auto rl : *mLights) {
			mConstraints.back()->addLight(rl.get());
		}
	}

	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();

	Eigen::VectorXf channelWeights; channelWeights.resize(entries_per_vertex);
	channelWeights.setOnes();

#ifdef IALT_USE_SPHERICAL_HARMONICS
	
	
	for (int l = 1; l <= sphericalHarmonicOrder; l++) {
		const float smoothing = 0.1f / (1.0f + 0.1 * float((l * (l + 1)) * (l * (l + 1)))); 
		for (int m = -l; m <= l; m++) {
			const int shIdx = (l * (l + 1) + m);
			channelWeights.segment(shIdx * 3, 3) *= smoothing;
		}
	}

	if (vars::useSHdiffOnlyCoeffObjective.value()) { channelWeights.setZero(); channelWeights(0) = channelWeights(1) = channelWeights(2) = 1.0; }
	if (vars::useSHdirOnlyCoeffObjective.value()) { channelWeights(0) = channelWeights(1) = channelWeights(2) = 1e-2; } 
	
	std::stringstream wstr; wstr << std::endl << "Objective SH weights: " << channelWeights.transpose() << std::endl;
	spdlog::info("{}", wstr.str());
#endif

	if (vars::objFuncOnGpu) {
		mChannelWeightsBuffer.STC_UploadData(&stc, channelWeights.data(), channelWeights.size() * sizeof(float));
		spdlog::info("Using GPU objective function evaluation");
		return;
	}

	Eigen::Matrix<float, -1, -1, Eigen::RowMajor> target; target.resize(static_cast<Eigen::Index>(mVertexCount), entries_per_vertex);
	Eigen::VectorXf targetWeights; targetWeights.resize(static_cast<Eigen::Index>(mVertexCount));
	Eigen::VectorXf vertexAreas; vertexAreas.resize(static_cast<Eigen::Index>(mVertexCount));
	Eigen::VectorXf vertexColor; vertexColor.resize(static_cast<Eigen::Index>(mVertexCount * 3));
	mTargetRadianceBuffer.STC_DownloadData(&stc, target.data());
	mTargetRadianceWeightsBuffer.STC_DownloadData(&stc, targetWeights.data());
	mVertexAreaBuffer.STC_DownloadData(&stc, vertexAreas.data());
	mVertexColorBuffer.STC_DownloadData(&stc, vertexColor.data());

	

	
	if (useHardcodedTargets) {
		spdlog::warn("Hardcoded targets are not supported anymore");
		/*if (Common::getInstance().getRenderSystem()->getMainScene()->getSceneFileName().find("Test-Office-") != std::string::npos) {
			TestOfficeTargetHardcoded(A, target, aScene, mVtxArea, entries_per_vertex);
			spdlog::info("Using hardcoded target for 'TestOffice'");
		}
		else if (Common::getInstance().getRenderSystem()->getMainScene()->getSceneFileName().find("twoplanes_lowres") != std::string::npos) {
			TwoPlanesLowResTargetHardcoded(A, target, aScene, mVtxArea, entries_per_vertex);
			spdlog::info("Using hardcoded target for 'TwoPlanesLowRes'");
		}
		else {
			A = mVtxArea.asDiagonal();
			target.setZero();
			spdlog::info("Using hardcoded ZERO target");
		}*/
	}

	
	
	if (mObjFcn) { delete mObjFcn; mObjFcn = NULL; }
	spdlog::info("Objective function evaluation on CPU");
	if(!(LightTraceOptimizer::vars::unphysicalNicePreview.asBool().value()) ){
		vertexColor.setOnes();
	}
	
	if( 0 /*consistent mass objective*/){ 
		mObjFcn = new ConsistentMassMultiChannelObjectiveFunction(targetWeights, mElems, mCoords, channelWeights, vertexColor, target); spdlog::info("consistent mass objective in use");
	}else{
		mObjFcn = new MultiChannelObjectiveFunction(targetWeights, vertexAreas, channelWeights, vertexColor, target);
	}	
}

void LightTraceOptimizer::copyRadianceToTarget() const
{
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	stc.begin();
	mRadianceBuffer.CMD_CopyBuffer(stc.buffer(), &mTargetRadianceBuffer);
	stc.end();
}

void LightTraceOptimizer::copyTargetToMesh(const tamashii::SceneBackendData& aScene) const
{
	std::vector<float> targetRadiance(mVertexCount * entries_per_vertex);
	std::vector<float> targetRadianceWeights(mVertexCount);
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	mTargetRadianceBuffer.STC_DownloadData(&stc, targetRadiance.data());
	mTargetRadianceWeightsBuffer.STC_DownloadData(&stc, targetRadianceWeights.data());

	size_t rIndex = 0;
	size_t wIndex = 0;
	for (const auto refModel : aScene.models) {
		for (const auto& refMesh : refModel->getMeshList()) {
			refMesh->hasColors0(true);
			if (entries_per_vertex != 3) {
				Mesh::CustomData* radData = refMesh->getCustomData(EXPORT_RADIANCE_ID);
				if (!radData) radData = refMesh->addCustomData(EXPORT_RADIANCE_ID);
				Mesh::CustomData* radInfoData = refMesh->getCustomData(EXPORT_RADIANCE_INFO_ID);
				if (!radInfoData) radInfoData = refMesh->addCustomData(EXPORT_RADIANCE_INFO_ID);

				auto* radInfoPtr = radInfoData->alloc<uint32_t>(2);
				radInfoPtr[0] = entries_per_vertex;
				radInfoPtr[1] = sphericalHarmonicOrder;

				auto* radPtr = radData->alloc<float>(refMesh->getVertexCount() * entries_per_vertex);
				std::memcpy(radPtr, rIndex + targetRadiance.data(), refMesh->getVertexCount() * entries_per_vertex * sizeof(float));
			}

			for (vertex_s& v : *refMesh->getVerticesVector()) {
				v.color_0 = glm::vec4(
					targetRadiance[rIndex + 0],
					targetRadiance[rIndex + 1],
					targetRadiance[rIndex + 2],
					targetRadianceWeights[wIndex]
				);
				rIndex += entries_per_vertex;
				wIndex++;
			}
		}
	}
}

void LightTraceOptimizer::copyMeshToTarget(const tamashii::SceneBackendData& aScene) const
{
	std::vector<float> targetRadiance(mVertexCount * entries_per_vertex);
	std::vector<float> targetRadianceWeights(mVertexCount);

	size_t rIndex = 0;
	size_t wIndex = 0;
	for (const auto refModel : aScene.models) {
		for (const auto& refMesh : refModel->getMeshList()) {
			if (entries_per_vertex != 3) {
				Mesh::CustomData* radData = refMesh->getCustomData(EXPORT_RADIANCE_ID);
				Mesh::CustomData* radInfoData = refMesh->getCustomData(EXPORT_RADIANCE_INFO_ID);
				if (radData && radInfoData) {
					const auto* radInfoPtr = radInfoData->data<uint32_t>();
					if (radInfoPtr[0] == entries_per_vertex || radInfoPtr[1] == sphericalHarmonicOrder) {
						const auto* radDataPtr = radData->data<float>();
						assert(radData->bytes() == refMesh->getVertexCount() * entries_per_vertex * sizeof(float));
						std::memcpy(targetRadiance.data() + rIndex, radDataPtr, refMesh->getVertexCount() * entries_per_vertex * sizeof(float));
					}
					else spdlog::warn("ialt: can not use radiance data because 'sh order' or 'entries per vertex' is different");
				}
			}

			for (const vertex_s& v : *refMesh->getVerticesVector()) {
				targetRadiance[rIndex + 0] = v.color_0.x;
				targetRadiance[rIndex + 1] = v.color_0.y;
				targetRadiance[rIndex + 2] = v.color_0.z;
				targetRadianceWeights[wIndex] = v.color_0.w;
				rIndex += entries_per_vertex;
				wIndex++;
			}
		}
	}
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	mTargetRadianceBuffer.STC_UploadData(&stc, targetRadiance.data());
	mTargetRadianceWeightsBuffer.STC_UploadData(&stc, targetRadianceWeights.data());
}

void LightTraceOptimizer::setTargetRadianceBufferForScene(std::optional<glm::vec3> c,
                                                          const std::optional<float> w) const
{
	std::vector<float> color(mVertexCount * entries_per_vertex);
	std::vector<float> weights(mVertexCount);

	size_t rIndex = 0;
	size_t wIndex = 0;
	for (const auto& refModel : *mModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			size_t cidx = rIndex;
			size_t widx = wIndex;
			for (const vertex_s& v : *refMesh->mesh->getVerticesVector()) {
				if (c.has_value()) {
					color[cidx + 0] = c->x;
					color[cidx + 1] = c->y;
					color[cidx + 2] = c->z;
				}
				if (w.has_value()) weights[widx] = w.value();
				cidx += entries_per_vertex;
				widx++;
			}
			

			rIndex = cidx;
			wIndex = widx;
		}
	}

	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	if (c.has_value()) mTargetRadianceBuffer.STC_UploadData(&stc, color.data(), mVertexCount * entries_per_vertex * sizeof(float));
	if (w.has_value()) mTargetRadianceWeightsBuffer.STC_UploadData(&stc, weights.data(), mVertexCount * sizeof(float));
	copyTargetToMesh(Common::getInstance().getRenderSystem()->getMainScene().get()->getSceneData());
}

void LightTraceOptimizer::setTargetRadianceBufferForMesh(const tamashii::RefMesh* mesh, const std::optional<glm::vec3> c, const std::optional<float> w) const
{
	size_t rIndex = 0;
	size_t wIndex = 0;
	for (const auto& refModel : *mModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			const size_t count = refMesh->mesh->getVertexCount();
			if(refMesh.get() == mesh)
			{
				const size_t byteOffsetColor = rIndex * sizeof(float);
				const size_t byteOffsetWeights = wIndex * sizeof(float);
				const size_t byteSizeColor = count * entries_per_vertex * sizeof(float);
				const size_t byteSizeWeights = count * sizeof(float);

				std::vector<float> color(count * entries_per_vertex);
				std::vector<float> weights(count);

				uint32_t cidx = 0;
				uint32_t widx = 0;
				for (const vertex_s& v : *refMesh->mesh->getVerticesVector()) {
					if (c.has_value()) {
						color[cidx + 0] = c->x;
						color[cidx + 1] = c->y;
						color[cidx + 2] = c->z;
					}
					if (w.has_value()) weights[widx] = w.value();
					cidx += entries_per_vertex;
					widx++;
				}

				rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
				if (c.has_value()) mTargetRadianceBuffer.STC_UploadData(&stc, color.data(), byteSizeColor, byteOffsetColor);
				if (w.has_value()) mTargetRadianceWeightsBuffer.STC_UploadData(&stc, weights.data(), byteSizeWeights, byteOffsetWeights);
				copyTargetToMesh(Common::getInstance().getRenderSystem()->getMainScene().get()->getSceneData());
				return;
			}

			rIndex += entries_per_vertex * count;
			wIndex += count;
		}
	}
}

rvk::Buffer* LightTraceOptimizer::getTargetRadianceBuffer()
{
	return &mTargetRadianceBuffer;
}

rvk::Buffer* LightTraceOptimizer::getTargetRadianceWeightsBuffer()
{
	return &mTargetRadianceWeightsBuffer;
}

rvk::Buffer* LightTraceOptimizer::getChannelWeightsBuffer()
{
	return &mChannelWeightsBuffer;
}

rvk::Descriptor* LightTraceOptimizer::getAdjointDescriptor()
{
	return &mAdjointDescriptor;
}

LightOptParams& LightTraceOptimizer::optimizationParametersByLightRef(const Ref& ref)
{
	updateLightParamsIfNecessary();

	
	auto it = mLightParams.find(const_cast<Ref*>(&ref));
	if (it == mLightParams.end()) {
		throw std::runtime_error{"No optimization parameters found for invalid light reference"};
	}

	return it->second;
}

rvk::Buffer* LightTraceOptimizer::getVtxTextureColorBuffer()
{
	return &mVertexColorBuffer;
}

rvk::Buffer* LightTraceOptimizer::getVtxAreaBuffer()
{
	return &mVertexAreaBuffer;
}

std::map<tamashii::Ref*, LightOptParams>& LightTraceOptimizer::getLightOptParams()
{
	return mLightParams;
}

void LightTraceOptimizer::sceneLoad(const tamashii::SceneBackendData aScene, const uint64_t aVertexCount)
{
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	mModels = &aScene.refModels;
	mLights = &aScene.refLights;
	mImages = &aScene.images;
	mTextures = &aScene.textures;
	mMaterials = &aScene.materials;
	mVertexCount = aVertexCount;

	
	mTriangleCount = 0;
	for (const auto refModel : aScene.refModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			mTriangleCount += refMesh->mesh->getPrimitiveCount();
		}
	}
	std::vector<uint32_t> triangleVector;
	triangleVector.reserve(mTriangleCount * 2u);
	uint32_t geoOffset = 0;
	for (const auto refModel : aScene.refModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			for (uint32_t i = 0; i < refMesh->mesh->getPrimitiveCount(); i++)
			{
				triangleVector.push_back(geoOffset);
				triangleVector.push_back(i);
			}
			geoOffset++;
		}
	}
	mTriangleBuffer.create(rvk::Buffer::Use::STORAGE, mTriangleCount * 2u * sizeof(uint32_t), rvk::Buffer::Location::DEVICE);
	mTriangleBuffer.STC_UploadData(&stc, triangleVector.data()); 
	

	vars::numRaysPerTriangle.value(std::max(1, static_cast<int>((vars::numRaysXperLight * vars::numRaysYperLight)/mTriangleCount)));

	/*mLightParams.clear();
	for (tamashii::RefLight* refLight : *mLights) mLightParams[refLight] = LightOptParams();*/
	importLightSettings();

	mRadianceBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::VERTEX, mVertexCount * entries_per_vertex * sizeof(float), rvk::Buffer::Location::DEVICE);
	mTargetRadianceBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::VERTEX, mVertexCount * entries_per_vertex * sizeof(float), rvk::Buffer::Location::DEVICE);
	mTargetRadianceWeightsBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::VERTEX, mVertexCount * sizeof(float), rvk::Buffer::Location::DEVICE);

	stc.begin();
	mRadianceBuffer.CMD_FillBuffer(stc.buffer(), 0);
	mTargetRadianceBuffer.CMD_FillBuffer(stc.buffer(), 0);
	mTargetRadianceWeightsBuffer.CMD_FillBuffer(stc.buffer(), 0);
	constexpr float weight = 1.0f;
	uint32_t oneFloatBits = 0;
	std::memcpy(&oneFloatBits, &weight, sizeof(float));
	mChannelWeightsBuffer.CMD_FillBuffer(stc.buffer(), oneFloatBits); 
	stc.end();

	
	copyMeshToTarget(aScene);

	
	mVertexAreaBuffer.create(rvk::Buffer::Use::STORAGE, mVertexCount * 1 * sizeof(float), rvk::Buffer::Location::DEVICE);

	sceneMeshToEigenArrays(aScene); 
	if (static_cast<Eigen::Index>(aVertexCount) != mCoords.rows()) spdlog::error("wrong vertex count -- possible Ref issue");
	mVtxArea.resize(mCoords.rows()); mVtxArea.setZero();
	for (uint32_t k = 0; k < mElems.rows(); ++k) {
		const uint32_t adof = mElems(k, 0);
		const uint32_t bdof = mElems(k, 1);
		const uint32_t cdof = mElems(k, 2);
		Eigen::Vector3f a(mCoords.row(adof)), b(mCoords.row(bdof)), c(mCoords.row(cdof));
		const float areaOver3 = (1.0f / (2.0f * 3.0f)) * ((b - a).cross(c - a)).norm();
		mVtxArea(adof) += areaOver3;
		mVtxArea(bdof) += areaOver3;
		mVtxArea(cdof) += areaOver3;
	}
	for (uint32_t i = 0; i < mCoords.rows(); ++i) { if( mVtxArea[i] < FLT_EPSILON ) mVtxArea[i]=FLT_EPSILON; }
	mVertexAreaBuffer.STC_UploadData(&stc, mVtxArea.data(), mVertexCount * 1 * sizeof(float));

	
	mVertexColorBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD, mVertexCount * 3 * sizeof(float), rvk::Buffer::Location::DEVICE);
	{
		union { float value = 0.0f; int data; } fill; fill.value = 0.0f;
		stc.begin();
		mVertexColorBuffer.CMD_FillBuffer(stc.buffer(), fill.data);
		stc.end();

		if(vars::unphysicalNicePreview) {
			
			rvk::CShader    cshader( &mRoot.device );
			rvk::Descriptor descriptor( &mRoot.device );
			rvk::CPipeline  cpipe( &mRoot.device );
			
			cshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::COMPUTE, "assets/shader/ialt/perVtxTexColor.glsl");
			cshader.finish();
			descriptor.addStorageBuffer(0, rvk::Shader::Stage::COMPUTE);
			descriptor.addStorageBuffer(1, rvk::Shader::Stage::COMPUTE);
			descriptor.addStorageBuffer(2, rvk::Shader::Stage::COMPUTE);
			descriptor.addStorageBuffer(3, rvk::Shader::Stage::COMPUTE);
			descriptor.addStorageBuffer(4, rvk::Shader::Stage::COMPUTE);
			descriptor.addStorageBuffer(5, rvk::Shader::Stage::COMPUTE);
			descriptor.setBuffer(0, &mVertexColorBuffer);
			descriptor.setBuffer(1, &mVertexAreaBuffer);
			descriptor.setBuffer(2, mGpuBlas->getIndexBuffer());
			descriptor.setBuffer(3, mGpuBlas->getVertexBuffer());
			descriptor.setBuffer(4, mGpuTlas->getGeometryDataBuffer());
			descriptor.setBuffer(5, mGpuMd->getMaterialBuffer());
			descriptor.finish();
			cpipe.addPushConstant(rvk::Shader::Stage::COMPUTE,0,sizeof(int));
			cpipe.addDescriptorSet({ mGpuTd->getDescriptor(), &descriptor });
			cpipe.setShader(&cshader);
			cpipe.finish();

			stc.begin();
			cpipe.CMD_BindDescriptorSets(stc.buffer(), { mGpuTd->getDescriptor(), &descriptor });
			cpipe.CMD_BindPipeline(stc.buffer());
			for(std::deque<std::shared_ptr<RefModel>>::iterator it=aScene.refModels.begin(); it!=aScene.refModels.end(); ++it){
				for(std::list<std::shared_ptr<RefMesh>>::iterator it2 = (*it)->refMeshes.begin(); it2!=(*it)->refMeshes.end(); ++it2){
					int geomIdx = mGpuBlas->getGeometryIndex((*it2)->mesh.get()) + mGpuTlas->getGeometryOffset(it->get());
					spdlog::info("Computing vertex-average texture color for mesh with geom-ID {} (idx-offset {}, vtx-offset {})", geomIdx, mGpuBlas->getOffset((*it2)->mesh.get()).mIndexOffset, mGpuBlas->getOffset((*it2)->mesh.get()).mVertexOffset);
					cpipe.CMD_SetPushConstant(stc.buffer(),rvk::Shader::Stage::COMPUTE,0,sizeof(int), &geomIdx);
					cpipe.CMD_Dispatch(stc.buffer(), (*it2)->mesh->getPrimitiveCount());
				}
			}
			stc.end();
		}
	}

	
	
	
	
	
	

	buildObjectiveFunction(aScene);

	lightsToParameterVector(mParams); 

	AdjointInfo_s afi{};
	afi.seed = mFwdSimCount = 0;
	afi.light_count = mGpuLd->getLightCount();
	afi.bounces = mBounces;
	afi.triangle_count = static_cast<uint32_t>(mTriangleCount);
	afi.tri_rays = vars::numRaysPerTriangle.value();
	afi.sam_rays = vars::numSamples.value();
	mInfoBuffer.create(rvk::Buffer::Use::STORAGE, sizeof(AdjointInfo_s), rvk::Buffer::Location::DEVICE);
	mInfoBuffer.STC_UploadData(&stc, &afi, sizeof(afi));

	mLightDerivativesBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD, std::max(1u, mGpuLd->getLightCount()) * sizeof(LightGrads), rvk::Buffer::Location::DEVICE);


	
	int meshCount = 0;
	mFirstEmissiveTexture=NULL;
	for(std::map<tamashii::Ref*, LightOptParams>::iterator it = mLightParams.begin(); it != mLightParams.end(); ++it) {
		if (it->first->type == Ref::Type::Mesh) {
			++meshCount;
			if( meshCount==1 ){
				RefMesh* refMesh = static_cast<RefMesh*>(it->first);
				mFirstEmissiveTexture = refMesh->mesh->getMaterial()->getEmissionTexture();
			}
		}
	}
	int texParamCount = 1; 
	if(meshCount>0){
		spdlog::info("found {} emissive meshes, texture optimization currently only supports first one, texture size = {}x{}",meshCount,mFirstEmissiveTexture->image->getWidth(),mFirstEmissiveTexture->image->getHeight());
		
		spdlog::info(" ... image format {} => {} bytes per pixel -- expect format={} (RGBA8_SRGB), 4 bytes per pixel",(int)( mFirstEmissiveTexture->image->getFormat() ), mFirstEmissiveTexture->image->getPixelSizeInBytes(), (int)Image::Format::RGBA8_SRGB);
		
		texParamCount = 3 * mFirstEmissiveTexture->image->getWidth() * mFirstEmissiveTexture->image->getHeight();
		
	}
	mLightTextureDerivativesBuffer.create(rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD, texParamCount * sizeof(double), rvk::Buffer::Location::DEVICE);

	lightTextureToParameterVector(mParams); 

	mAdjointDescriptor.setAccelerationStructureKHR(ADJOINT_DESC_TLAS_BINDING, mGpuTlas->getTlas());
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_INFO_BUFFER_BINDING, &mInfoBuffer);
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_RADIANCE_BUFFER_BINDING, &mRadianceBuffer);
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_INDEX_BUFFER_BINDING, mGpuBlas->getIndexBuffer());
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_VERTEX_BUFFER_BINDING, mGpuBlas->getVertexBuffer());
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_MATERIAL_BUFFER_BINDING, mGpuMd->getMaterialBuffer());
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_GEOMETRY_BUFFER_BINDING, mGpuTlas->getGeometryDataBuffer());
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_LIGHT_BUFFER_BINDING, mGpuLd->getLightBuffer());
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_AREA_BUFFER_BINDING, &mVertexAreaBuffer);
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_LIGHT_DERIVATIVES_BUFFER_BINDING, &mLightDerivativesBuffer);
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_LIGHT_TEXTURE_DERIVATIVES_BUFFER_BINDING, &mLightTextureDerivativesBuffer);
	mAdjointDescriptor.setBuffer(ADJOINT_DESC_TRIANGLE_BUFFER_BINDING, &mTriangleBuffer);
	mAdjointDescriptor.update();

	mObjFuncDescriptor.setBuffer(OBJ_DESC_RADIANCE_BUFFER_BINDING, &mRadianceBuffer);
	mObjFuncDescriptor.setBuffer(OBJ_DESC_TARGET_RADIANCE_BUFFER_BINDING, &mTargetRadianceBuffer);
	mObjFuncDescriptor.setBuffer(OBJ_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, &mTargetRadianceWeightsBuffer);
	mObjFuncDescriptor.setBuffer(OBJ_DESC_AREA_BUFFER_BINDING, &mVertexAreaBuffer);
	mObjFuncDescriptor.setBuffer(OBJ_DESC_CHANNEL_WEIGHTS_BUFFER_BINDING, &mChannelWeightsBuffer);
	mObjFuncDescriptor.setBuffer(OBJ_DESC_PHI_BUFFER_BINDING, &mPhiBuffer);
	mObjFuncDescriptor.setBuffer(OBJ_DESC_VERTEX_COLOR_BUFFER_BINDING, &mVertexColorBuffer);
	mObjFuncDescriptor.update();

	clearHistory();
	mSceneReady = true;
}

void LightTraceOptimizer::sceneUnload(tamashii::SceneBackendData aScene)
{
	mSceneReady = false;
	mRadianceBuffer.destroy();
	mTargetRadianceBuffer.destroy();
	mTargetRadianceWeightsBuffer.destroy();
	mVertexAreaBuffer.destroy();
	mVertexColorBuffer.destroy();
	mLightDerivativesBuffer.destroy();
	mLightTextureDerivativesBuffer.destroy();
	mTriangleBuffer.destroy();
}

void LightTraceOptimizer::destroy()
{
	delete mObjFcn; mObjFcn = NULL;

	mAdjointDescriptor.destroy();
	mObjFuncDescriptor.destroy();
	mForwardShader.destroy();
	mForwardPTShader.destroy();
	mForwardPipeline.destroy();
	mForwardPTPipeline.destroy();
	mBackwardShader.destroy();
	mBackwardPipeline.destroy();
	mBackwardPTShader.destroy();
	mBackwardPTPipeline.destroy();
	mObjFuncShader.destroy();
	mObjFuncPipeline.destroy();
	mInfoBuffer.destroy();
	mChannelWeightsBuffer.destroy();
	mPhiBuffer.destroy();
	mCpuBuffer.destroy();

	tamashii::FileWatcher::getInstance().removeFile(IALT_SHADER_DIR "forward_rgen.glsl");
}

void LightTraceOptimizer::importLightSettings()
{
	mLightParams.clear();
	for (auto& refLight : *mLights) {
		LightOptParams lop = LightOptParams();
		Value settings = refLight->light->getCustomProperty("optimizer_settings");
		if (!settings.isEmpty() && settings.isMap())
		{
			lop.reset();
			std::map<std::string, Value> m = settings.getMap();
			if (m.count("pos_x")) lop[LightOptParams::POS_X] = m["pos_x"].getBool();
			if (m.count("pos_y")) lop[LightOptParams::POS_Y] = m["pos_y"].getBool();
			if (m.count("pos_z")) lop[LightOptParams::POS_Z] = m["pos_z"].getBool();
			if (m.count("intensity")) lop[LightOptParams::INTENSITY] = m["intensity"].getBool();
			if (m.count("rot_x")) lop[LightOptParams::ROT_X] = m["rot_x"].getBool();
			if (m.count("rot_y")) lop[LightOptParams::ROT_Y] = m["rot_y"].getBool();
			if (m.count("rot_z")) lop[LightOptParams::ROT_Z] = m["rot_z"].getBool();
			if (m.count("cone_inner")) lop[LightOptParams::CONE_INNER] = m["cone_inner"].getBool();
			if (m.count("cone_outer")) lop[LightOptParams::CONE_EDGE] = m["cone_outer"].getBool();
			if (m.count("color_r")) lop[LightOptParams::COLOR_R] = m["color_r"].getBool();
			if (m.count("color_g")) lop[LightOptParams::COLOR_G] = m["color_g"].getBool();
			if (m.count("color_b")) lop[LightOptParams::COLOR_B] = m["color_b"].getBool();
		}
		mLightParams[refLight.get()] = lop;
	}
	for (const auto& refModel : *mModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			if (refMesh->mesh->getMaterial()->isLight()) {
				LightOptParams lop = LightOptParams();
				Value settings = refMesh->mesh->getCustomProperty("optimizer_settings");
				if (!settings.isEmpty() && settings.isMap())
				{
					lop.reset();
					std::map<std::string, Value> m = settings.getMap();
					if (m.count("emission_strength")) lop[LightOptParams::INTENSITY] = m["emission_strength"].getBool();
					if (m.count("emission_color_r")) lop[LightOptParams::COLOR_R] = m["emission_color_r"].getBool();
					if (m.count("emission_color_g")) lop[LightOptParams::COLOR_G] = m["emission_color_g"].getBool();
					if (m.count("emission_color_b")) lop[LightOptParams::COLOR_B] = m["emission_color_b"].getBool();
				}
				mLightParams[refMesh.get()] = lop;
			}
		}
	}
}

void LightTraceOptimizer::exportLightSettings()
{
	for (auto& refLight : *mLights) {
		LightOptParams lop = mLightParams[refLight.get()];
		std::map<std::string, Value> m;

		m["pos_x"] = Value(lop[LightOptParams::POS_X]);
		m["pos_y"] = Value(lop[LightOptParams::POS_Y]);
		m["pos_z"] = Value(lop[LightOptParams::POS_Z]);
		m["intensity"] = Value(lop[LightOptParams::INTENSITY]);
		m["rot_x"] = Value(lop[LightOptParams::ROT_X]);
		m["rot_y"] = Value(lop[LightOptParams::ROT_Y]);
		m["rot_z"] = Value(lop[LightOptParams::ROT_Z]);
		m["cone_inner"] = Value(lop[LightOptParams::CONE_INNER]);
		m["cone_outer"] = Value(lop[LightOptParams::CONE_EDGE]);
		m["color_r"] = Value(lop[LightOptParams::COLOR_R]);
		m["color_g"] = Value(lop[LightOptParams::COLOR_G]);
		m["color_b"] = Value(lop[LightOptParams::COLOR_B]);

		refLight->light->addCustomProperty("optimizer_settings", Value(m));
	}
	for (const auto& refModel : *mModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			if (refMesh->mesh->getMaterial()->isLight()) {
				LightOptParams lop = mLightParams[refMesh.get()];
				std::map<std::string, Value> m;

				m["emission_strength"] = Value(lop[LightOptParams::INTENSITY]);
				m["emission_color_r"] = Value(lop[LightOptParams::COLOR_R]);
				m["emission_color_g"] = Value(lop[LightOptParams::COLOR_G]);
				m["emission_color_b"] = Value(lop[LightOptParams::COLOR_B]);

				refMesh->mesh->addCustomProperty("optimizer_settings", Value(m));
			}
		}
	}
}

LBFGSppWrapperResult LightTraceOptimizer::optimize(const uint32_t aOptimizer, rvk::Buffer* aRadianceBufferOut, float aStepSize, int aMaxIters) {
	if (!mGpuLd->getLightCount())
		return { 0, 0 };

	optimizationRunning(true);
	mForwardTimeCount = 0;
	mBackwardTimeCount = 0;
	mForwardTimeSum = 0;
	mBackwardTimeSum = 0;
	const auto start = std::chrono::high_resolution_clock::now();
	clearHistory();
	
	if (mLightDerivativesBuffer.getSize() < sizeof(LightGrads) * mGpuLd->getLightCount()) {
		
		mLightDerivativesBuffer.destroy();
		mLightDerivativesBuffer.create(rvk::Buffer::Use::STORAGE, mGpuLd->getLightCount() * sizeof(LightGrads), rvk::Buffer::Location::DEVICE);
		mAdjointDescriptor.setBuffer(ADJOINT_DESC_LIGHT_DERIVATIVES_BUFFER_BINDING, &mLightDerivativesBuffer);
		mAdjointDescriptor.update();
	}

	lightsToParameterVector(mParams); 

	lightTextureToParameterVector(mParams); 

	uint32_t iters = 0;
	uint32_t evals = 0;
	float objective = 0;
	OptimWrapperBase<Eigen::VectorXd>::OptimizationResult result;
	if (aOptimizer == LBFGS) {
		spdlog::info("using L-BFGS");
		LBFGSppWrapper<Eigen::VectorXd> solver(this, aRadianceBufferOut);
		solver.mMaxIters = aMaxIters;
		solver.mOptimOptions.init_step = static_cast<double>(aStepSize);
		result= solver.runOptimization(mParams);
		iters = solver.mIters; evals = solver.mEvals; objective = solver.mBestObjectiveValue;
		spdlog::info("L-BFGS finished, {} iters, {} evals, best objective {}, grad norm {}", solver.mIters, solver.mEvals, solver.mBestObjectiveValue, solver.mBestRunGradient.norm());
	}
	else if (aOptimizer == GD) {
		spdlog::info("using GD");
		SimpleGradientDescentWrapper<Eigen::VectorXd> solver(this, aRadianceBufferOut);
		solver.mMaxIters = aMaxIters;
		solver.mStepSize = static_cast<double>(aStepSize);
		result = solver.runOptimization(mParams);
		iters = solver.mIters; evals = solver.mEvals; objective = solver.mBestObjectiveValue;
		spdlog::info("GD finished, {} iters, {} evals, best objective {}, grad norm {}", solver.mIters, solver.mEvals, solver.mBestObjectiveValue, solver.mBestRunGradient.norm());
	}
	else if (aOptimizer == ADAM) {
		spdlog::info("using ADAM");
		AdamWrapper<Eigen::VectorXd> solver(this, aRadianceBufferOut);
		solver.mMaxIters = aMaxIters;
		solver.mStepSize = static_cast<double>(aStepSize);
		result = solver.runOptimization(mParams);
		iters = solver.mIters; evals = solver.mEvals; objective = solver.mBestObjectiveValue;
		spdlog::info("ADAM finished, {} iters, {} evals, best objective {}, grad norm {}", solver.mIters, solver.mEvals, solver.mBestObjectiveValue, solver.mBestRunGradient.norm());
	}
	else if (aOptimizer == FD_FORWARD) {
		spdlog::info("running forward finite difference check");
		FiniteDifferenceForwardCheckWrapper<Eigen::VectorXd> solver(this, aRadianceBufferOut);
		solver.mMaxIters = aMaxIters;
		solver.mStepSize = static_cast<double>(aStepSize);
		result = solver.runOptimization(mParams);

		std::stringstream pstr; pstr << std::endl << "Forward FD-check ..." << std::endl <<
			"supplied grad. = [ " << solver.mBestRunParameters.transpose() << " ]" << std::endl <<
			"fin.diff grad. = [ " << solver.mBestRunGradient.transpose() << " ]" << std::endl <<
			"difference norm = " << solver.mBestObjectiveValue << " (rel: " << (solver.mBestObjectiveValue / solver.mBestRunParameters.norm()) << ")" << std::endl;
		spdlog::info("{}", pstr.str());
	}
	else if (aOptimizer == FD_CENTRAL) {
		spdlog::info("running central finite difference check");
		FiniteDifferenceCentralCheckWrapper<Eigen::VectorXd> solver(this, aRadianceBufferOut);
		solver.mMaxIters = aMaxIters;
		solver.mStepSize = static_cast<double>(aStepSize);
		result = solver.runOptimization(mParams);

		std::stringstream pstr; pstr << std::endl << "Central FD-check ..." << std::endl <<
			"supplied grad. = [ " << solver.mBestRunParameters.transpose() << " ]" << std::endl <<
			"fin.diff grad. = [ " << solver.mBestRunGradient.transpose() << " ]" << std::endl <<
			"difference norm = " << solver.mBestObjectiveValue << " (rel: " << (solver.mBestObjectiveValue / solver.mBestRunParameters.norm()) << ")" << std::endl;
		spdlog::info("{}", pstr.str());
	}
	else if (aOptimizer == FD_CENTRAL_ADAM) {
		spdlog::info("central finite difference ADAM");
		FiniteDifferenceCentralAdamWrapper<Eigen::VectorXd> solver(this, aRadianceBufferOut);
		solver.mMaxIters = aMaxIters;
		solver.mStepSize = static_cast<double>(aStepSize);
		result = solver.runOptimization(mParams);
		iters = solver.mIters; evals = solver.mEvals; objective = solver.mBestObjectiveValue;
		spdlog::info("ADAM finished, {} iters, {} evals, best objective {}, grad norm {}", solver.mIters, solver.mEvals, solver.mBestObjectiveValue, solver.mBestRunGradient.norm());
	}
	else if (aOptimizer == CMA_ES) {
		spdlog::info("using CMA-ES (not using gradients!)");
		CMAWrapper solver(this, aRadianceBufferOut);
		solver.mInitStdDev = static_cast<double>(aStepSize);
		solver.mMaxIters = aMaxIters;
		result = solver.runOptimization(mParams);
		iters = solver.mIters; evals = solver.mEvals; objective = solver.mBestObjectiveValue;
		spdlog::info("CMA-ES finished, {} evals, best objective {}", solver.mEvals, solver.mBestObjectiveValue);
	}
	else {
		spdlog::info("optimizer not implemented {}", aOptimizer);
	}

	auto time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);

	spdlog::info("Stats:");
	spdlog::info("\tavg. forward time:\t\t{} milliseconds", (mForwardTimeSum / mForwardTimeCount) / 1000.0f);
	spdlog::info("\tavg. backward time:\t\t{} milliseconds", (mBackwardTimeSum / mBackwardTimeCount) / 1000.0f);
	spdlog::info("\ttotal time:\t\t\t{} seconds ", static_cast<float>(time.count()) / 1000000.0f);
	if(vars::usePathTracing[0]) spdlog::info("\tpt forward rays spawned per iteration:\t{} rays", vars::numRaysPerTriangle * mLights->size() * mTriangleCount);
	else spdlog::info("\tlt forward rays spawned per iteration:\t{} rays", vars::numRaysXperLight * vars::numRaysYperLight * mLights->size());
	if (vars::usePathTracing[1]) spdlog::info("\tpt backward rays spawned per iteration:\t{} rays", vars::numRaysPerTriangle * mLights->size() * mTriangleCount);
	else spdlog::info("\tlt backward rays spawned per iteration:\t{} rays", vars::numRaysXperLight * vars::numRaysYperLight * mLights->size());
	spdlog::info("\toptimizer:\t\t\t{}", optimizerNames[aOptimizer]);
	spdlog::info("\tstepsize:\t\t\t{}", aStepSize);
	spdlog::info("\titerations:\t\t\t{}", iters);
	spdlog::info("\tevaluations:\t\t\t{}", evals);
	spdlog::info("\tbest objective:\t\t\t{}", objective);
	spdlog::info("\tsh order:\t\t\t{}", sphericalHarmonicOrder);
	spdlog::info("\tvertex count:\t\t\t{}", mVertexCount);
	spdlog::info("\ttriangle count:\t\t\t{}", mTriangleCount);
	spdlog::info("\tlight count:\t\t\t{}", mLights->size());

	forward(mParams, aRadianceBufferOut); 
	optimizationRunning(false);

	return { .bestObjectiveValue = result.bestObjectiveValue, .lastPhi = result.lastPhi };
}

void LightTraceOptimizer::optimizationRunning(const bool aRun)
{
	std::lock_guard guard(mOptimizationMutex); mOptimizationRunning = aRun;
}

bool LightTraceOptimizer::optimizationRunning()
{
	std::lock_guard guard(mOptimizationMutex); return mOptimizationRunning;
}

void LightTraceOptimizer::forward(Eigen::VectorXd& aParams, rvk::Buffer* aRadianceBufferOut)
{
	if (!mSceneReady || !mGpuLd->getLightCount()) return;
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();

	std::stringstream pstr; pstr << aParams.segment(0,std::min(aParams.size(),(Eigen::Index)20)).transpose();
	spdlog::info("\n\n params = [{}]\n\n", pstr.str());
	if( aParams.size()>20 ) spdlog::info("(showing first 20 elements only)");


	parameterVectorToLights(aParams);
	parameterVectorToLightTexture(aParams);

	AdjointInfo_s afi{};
	afi.seed = mFwdSimCount;
	if (!vars::constRandSeed) ++mFwdSimCount;
	afi.light_count = mGpuLd->getLightCount();
	afi.bounces = mBounces;
	afi.triangle_count = static_cast<uint32_t>(mTriangleCount);
	afi.tri_rays = vars::numRaysPerTriangle.value();
	afi.sam_rays = vars::numSamples.value();
	std::memcpy(mCpuBuffer.getMemoryPointer(), &afi, sizeof(afi));

	const auto start = std::chrono::high_resolution_clock::now();
	stc.begin();
	mCpuBuffer.CMD_CopyBuffer(stc.buffer(), &mInfoBuffer, 0u, sizeof(AdjointInfo_s));
	stc.buffer()->cmdBufferMemoryBarrier(&mInfoBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	mRadianceBuffer.CMD_FillBuffer(stc.buffer(), 0);
	stc.buffer()->cmdBufferMemoryBarrier(&mRadianceBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);

	if (mForwardPT) {
		mForwardPTPipeline.CMD_BindDescriptorSets(stc.buffer(), { mGpuTd->getDescriptor(), &mAdjointDescriptor });
		mForwardPTPipeline.CMD_BindPipeline(stc.buffer());
		mForwardPTPipeline.CMD_TraceRays(stc.buffer(), mTriangleCount, vars::numRaysPerTriangle, static_cast<uint32_t>(mGpuLd->getLightCount()));
	}
	else
	{
		mForwardPipeline.CMD_BindDescriptorSets(stc.buffer(), { mGpuTd->getDescriptor(), &mAdjointDescriptor });
		mForwardPipeline.CMD_BindPipeline(stc.buffer());
		mForwardPipeline.CMD_TraceRays(stc.buffer(), vars::numRaysXperLight, vars::numRaysYperLight, static_cast<uint32_t>(mGpuLd->getLightCount()));
	}
	
	
	
	
	
	
	
	
	
	
	
	
	

	if (aRadianceBufferOut) {
		stc.buffer()->cmdBufferMemoryBarrier(&mRadianceBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
		mRadianceBuffer.CMD_CopyBuffer(stc.buffer(), aRadianceBufferOut);

	}
	stc.end();
	mForwardTimeCount++;
	mForwardTimeSum += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
	


	
	if (0/* && IALT_USE_SPHERICAL_HARMONICS*/) {
		spdlog::info("*** (H)SH hard-coded test - overwriting results in LightTraceOptimizer::forward(...) ***");
		Eigen::VectorXf singleSHdata; singleSHdata.resize((sphericalHarmonicOrder + 1) * (sphericalHarmonicOrder + 1));
		singleSHdata.setZero(); singleSHdata[0] = 1.0; singleSHdata[2] = 0.7; singleSHdata[6] = -0.2;
		
		Eigen::MatrixXf rgbSHdata; rgbSHdata.resize(3, (sphericalHarmonicOrder + 1) * (sphericalHarmonicOrder + 1));
		rgbSHdata.row(0) = singleSHdata;
		rgbSHdata.row(1) = singleSHdata;
		rgbSHdata.row(2) = singleSHdata;
		
		Eigen::VectorXf allSHdata; allSHdata.resize(entries_per_vertex * mCoords.rows()); allSHdata.setZero();
		for (unsigned int k = 0; k < mCoords.rows(); ++k) allSHdata.segment(k * entries_per_vertex, entries_per_vertex) = rgbSHdata;
		
		aRadianceBufferOut->STC_UploadData(&stc, allSHdata.data(), allSHdata.size() * sizeof(float));
	}

	
	const bool debugOutput = false; 
	if (debugOutput && aRadianceBufferOut) {
		Eigen::VectorXf x; x.resize(mCoords.rows() * entries_per_vertex); x.setZero();
		aRadianceBufferOut->STC_DownloadData(&stc, x.data(), x.size() * sizeof(float));
		Eigen::Map<Eigen::Matrix<float, -1, -1, Eigen::RowMajor> > radianceData(x.data(), mCoords.rows(), entries_per_vertex);
		std::ofstream ptFile("points.txt");
		for (size_t j = 0; j < mCoords.rows(); ++j) {
			Eigen::Vector3f p = mCoords.row(j);
			ptFile << p[0] << " " << p[1] << " " << p[2] << std::endl;
		}
		ptFile.close();

		std::ofstream axFile("axis.txt");
		axFile << mVertexNormalTangent << std::endl;
		axFile.close();

		std::ofstream shFile("radiance.txt");
		shFile << radianceData << std::endl;
		shFile.close();
	}
	
	
	
	
	
	
	
}

double LightTraceOptimizer::backward(Eigen::VectorXd& aDerivParams)
{
	if (!mSceneReady || !mGpuLd->getLightCount()) return 0;
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();

	const auto start = std::chrono::high_resolution_clock::now();

	
	double phi = 0.0;

	if (!vars::objFuncOnGpu) {
		Eigen::VectorXf dx, x;
		x.resize(static_cast<Eigen::Index>(mVertexCount * entries_per_vertex)); x.setZero(); 
		dx.resizeLike(x); dx.setZero();

		mRadianceBuffer.STC_DownloadData(&stc, x.data(), x.size() * sizeof(float)); 

		
		

		
		phi = static_cast<double>((*mObjFcn)(x, dx)); 

		
		
		

		
		

		
		
		
		

		
		mRadianceBuffer.STC_UploadData(&stc, dx.data(), dx.size() * sizeof(float)); 
		stc.begin();
	}
	else {
		stc.begin();
		mPhiBuffer.CMD_FillBuffer(stc.buffer(), 0);
		stc.buffer()->cmdBufferMemoryBarrier(&mPhiBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
		mObjFuncPipeline.CMD_BindDescriptorSets(stc.buffer(), { &mObjFuncDescriptor });
		mObjFuncPipeline.CMD_BindPipeline(stc.buffer());
		const auto vertexCount = static_cast<uint32_t>(mVertexCount);
		mObjFuncPipeline.CMD_SetPushConstant(stc.buffer(), rvk::Shader::Stage::COMPUTE, 0, sizeof(uint32_t), &vertexCount);
		const uint32_t dispatchSizeX = (vertexCount / OBJ_FUNC_WORKGROUP_SIZE) + (vertexCount % OBJ_FUNC_WORKGROUP_SIZE ? 1u : 0u);
		mObjFuncPipeline.CMD_Dispatch(stc.buffer(), dispatchSizeX, entries_per_vertex);
		stc.buffer()->cmdBufferMemoryBarrier(&mPhiBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
		mPhiBuffer.CMD_CopyBuffer(stc.buffer(), &mCpuBuffer);
		stc.buffer()->cmdBufferMemoryBarrier(&mRadianceBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	}

	mLightDerivativesBuffer.CMD_FillBuffer(stc.buffer(), 0); 
	stc.buffer()->cmdBufferMemoryBarrier(&mLightDerivativesBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

	mLightTextureDerivativesBuffer.CMD_FillBuffer(stc.buffer(), 0); 
	stc.buffer()->cmdBufferMemoryBarrier(&mLightTextureDerivativesBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);


	if (mBackwardPT) {
		mBackwardPTPipeline.CMD_BindDescriptorSets(stc.buffer(), { mGpuTd->getDescriptor(), &mAdjointDescriptor });
		mBackwardPTPipeline.CMD_BindPipeline(stc.buffer());
		mBackwardPTPipeline.CMD_TraceRays(stc.buffer(), mTriangleCount, vars::numRaysPerTriangle, mGpuLd->getLightCount());
	}
	else
	{
		mBackwardPipeline.CMD_BindDescriptorSets(stc.buffer(), { mGpuTd->getDescriptor(), &mAdjointDescriptor });
		mBackwardPipeline.CMD_BindPipeline(stc.buffer());
		mBackwardPipeline.CMD_TraceRays(stc.buffer(), vars::numRaysXperLight, vars::numRaysYperLight, mGpuLd->getLightCount());
	}

	stc.end();
	if (vars::objFuncOnGpu) {
		const auto phiPtr = reinterpret_cast<double*>(mCpuBuffer.getMemoryPointer());
		phi = *phiPtr;
	}

	lightDerivativesToVector(aDerivParams);

	
	double phiC = 0.0;
	for (LightConstraint* lc : mConstraints)
	{
		phiC += lc->evalAndAddToGradient(aDerivParams); 
	}

	const Eigen::VectorXd derivParams = aDerivParams;
	LightOptParams::reduceVectorToActiveParams(aDerivParams, derivParams, mLightParams);

	double phiT = lightTextureDerivativesToVector(aDerivParams); 

	mBackwardTimeCount++;
	mBackwardTimeSum += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

	
	if (!std::isfinite(phi) || !std::isfinite(phiC)) { phi = DBL_MAX; phiC = 0.0; }
	for (int i = 0; i < aDerivParams.size(); ++i) { if (!std::isfinite(aDerivParams[i])) aDerivParams[i] = 0.0; }

	spdlog::info("objective + constraints = {} + {} + {}", phi, phiC, phiT);
	std::stringstream pstr; pstr << aDerivParams.segment(0,std::min(aDerivParams.size(),(Eigen::Index)20)).transpose();
	spdlog::info("\n\ndparams = [{}]\n\n", pstr.str());
	if( aDerivParams.size()>20 ) spdlog::info("(showing first 20 elements only)");

	return phi + phiC + phiT;
}

void LightTraceOptimizer::addCurrentStateToHistory(const Eigen::VectorXd& aParams)
{
	mCurrentHistoryIndex++;
	mHistory.push_back(aParams);
}

uint32_t LightTraceOptimizer::getHistorySize() const
{
	return static_cast<uint32_t>(mHistory.size());
}

int& LightTraceOptimizer::getCurrentHistoryIndex()
{
	return mCurrentHistoryIndex;
}

void LightTraceOptimizer::selectParamsFromHistory(const int aIndex)
{
	parameterVectorToLights(mHistory[aIndex]);
}

void LightTraceOptimizer::clearHistory()
{
	mCurrentHistoryIndex = -1;
	mHistory.clear();
	
}

std::deque<Eigen::VectorXd> LightTraceOptimizer::exportHistory()
{
	auto hist = std::move(mHistory);
	clearHistory();
	return hist;
}

void LightTraceOptimizer::fillFiniteDiffRadianceBuffer(const tamashii::RefLight* aRefLight, const LightOptParams::PARAMS aParam, const float aH,
	rvk::Buffer* aFdRadianceBufferOut, rvk::Buffer* aFd2RadianceBufferOut)
{
	const uint32_t lightIdx = LightOptParams::getParameterIndex(aRefLight->ref_light_index, aParam);
	updateParamsFromScene();
	Eigen::VectorXd oldParams = Eigen::VectorXd(getCurrentParams());
	Eigen::VectorXd& currentParams = getCurrentParams();
	currentParams(lightIdx) = oldParams(lightIdx) + static_cast<double>(aH);
	forward(currentParams, aFdRadianceBufferOut);
	currentParams(lightIdx) = oldParams(lightIdx) - static_cast<double>(aH);
	forward(currentParams, aFd2RadianceBufferOut);
	forward(oldParams);
}

void LightTraceOptimizer::updateLightParamsIfNecessary()
{
	if (mLightParams.size() != mGpuLd->getLightCount()) {
		mLightParams.clear();
		spdlog::warn("rebuilding light parameter map ...");
		uint32_t count = 0;
		for (const auto& refLight : *mLights) {
			mLightParams[refLight.get()] = LightOptParams();
			
			spdlog::info("... light {} has index {}", count, refLight->ref_light_index);
			count++;
		}
		for (const auto& refModel : *mModels) {
			for (const auto& refMesh : refModel->refMeshes) {
				if (refMesh->mesh->getMaterial()->isLight()) {
					mLightParams[refMesh.get()] = LightOptParams();
					
					spdlog::info("... light {} has index {}", count, count);
					count++;
				}
			}
		}
	}
}

void LightTraceOptimizer::sceneMeshToEigenArrays(const SceneBackendData& aScene) {
	
	constexpr int nCoordsPerNode = 3;
	constexpr int nNodesPerElem = 3;
	uint32_t nNodes = 0;
	uint32_t nElems = 0;
	std::map<uint32_t, uint32_t> nodeIDoffsetPerMesh; uint32_t meshCount = 0;
	for (const auto refModel : aScene.refModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			nodeIDoffsetPerMesh[meshCount] = nNodes;
			nNodes += refMesh->mesh->getVertexCount();
			nElems += refMesh->mesh->getPrimitiveCount();
			++meshCount;
		}
	}

	mCoords.resize(nNodes, nCoordsPerNode);
	mElems.resize(nElems, nNodesPerElem);
	mVertexNormalTangent.resize(nNodes, 2*nCoordsPerNode);

	uint32_t nextNode = 0;
	for (const auto refModel : aScene.refModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			glm::mat4 modelMatrix = refModel->model_matrix; 
			for (uint32_t j = 0; j < refMesh->mesh->getVertexCount(); ++j) {
				glm::vec4 p = modelMatrix * refMesh->mesh->getVerticesArray()[j].position;
				mCoords(nextNode, 0) = p[0];
				mCoords(nextNode, 1) = p[1];
				mCoords(nextNode, 2) = p[2];

				glm::vec4 normal = glm::vec4(glm::mat3(transpose(inverse(modelMatrix))) * glm::vec3(refMesh->mesh->getVerticesArray()[j].normal ), 1.0f);
				glm::vec4 tangent= glm::vec4(glm::mat3(transpose(inverse(modelMatrix))) * glm::vec3(refMesh->mesh->getVerticesArray()[j].tangent), 1.0f);

				mVertexNormalTangent(nextNode, 0) = normal[0];
				mVertexNormalTangent(nextNode, 1) = normal[1];
				mVertexNormalTangent(nextNode, 2) = normal[2];
				mVertexNormalTangent(nextNode, 3) = tangent[0];
				mVertexNormalTangent(nextNode, 4) = tangent[1];
				mVertexNormalTangent(nextNode, 5) = tangent[2];

				++nextNode;
			}
		}
	}

	meshCount = 0;
	uint32_t nextElem = 0;
	for (const auto refModel : aScene.refModels) {
		for (const auto& refMesh : refModel->refMeshes) {
			for (uint32_t j = 0; j < refMesh->mesh->getPrimitiveCount(); ++j) {
				for (uint32_t i = 0; i < nNodesPerElem; ++i) {
					mElems(nextElem, i) = refMesh->mesh->getIndicesArray()[nNodesPerElem * j + i] + nodeIDoffsetPerMesh[meshCount];
				}
				++nextElem;
			}
			++meshCount;
		}
	}
}

void LightTraceOptimizer::writeLegacyVTKpointData(Eigen::MatrixXi& aElems, Eigen::MatrixXf& aCoords, const rvk::Buffer* aDataBuffer,
	const std::string& aFilename, const std::string& aDataname) {
	
	

	const size_t nCoordsPerNode = aCoords.cols();
	const size_t nNodesPerElem = aElems.cols();
	const size_t nNodes = aCoords.rows();
	const size_t nElems = aElems.rows();
	std::ofstream out(aFilename.c_str());
	if (!out.is_open()) {
		spdlog::error("Failed to open ofstream {}", aFilename);
		return;
	}
	spdlog::info("Writing {} with {}", aFilename, aDataname);

	out.precision(12);
	out.setf(std::ios::scientific);

	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	Eigen::Matrix<float, -1, -1, Eigen::RowMajor> pointData; pointData.resize(nNodes, entries_per_vertex); pointData.setZero(); 
	aDataBuffer->STC_DownloadData(&stc, pointData.data(), pointData.size() * sizeof(float)); 
	

	
	out << "# vtk DataFile Version 2.0" << std::endl;
	out << "VTK exported mesh" << std::endl;
	out << "ASCII" << std::endl;
	out << "DATASET UNSTRUCTURED_GRID" << std::endl;
	
	out << "POINTS " << nNodes << " double" << std::endl;
	for (size_t j = 0; j < nNodes; ++j) {
		Eigen::Vector3f p = aCoords.row(j);
		out << p[0] << " " << p[1] << " " << p[2] << std::endl;
	}
	
	out << "CELLS " << nElems << " " << (1 + nNodesPerElem) * nElems << std::endl;
	for (size_t j = 0; j < nElems; ++j) {
		out << nNodesPerElem << " " << aElems.row(j) << std::endl;
	}

	
	out << "CELL_TYPES " << nElems << std::endl;
	for (size_t i = 0; i < nElems; ++i) out << /*VTK_TRIANGLE =*/ 5 << std::endl;
	

	out << "POINT_DATA " << nNodes << std::endl;
	
	
	out << "VECTORS " << aDataname << " float" << std::endl;
	for (size_t j = 0; j < nNodes; ++j) out << pointData(j, 0) << " " << pointData(j, 1) << " " << pointData(j, 2) << std::endl;

	out.close();
}

void LightTraceOptimizer::lightsToParameterVector(Eigen::VectorXd& aParams) {
	updateLightParamsIfNecessary();

	{	
		
		
		
		
		

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		
		

		
		

		
		

		
		
		
		

		
		
		

		
		
		
		
		

		
		
		
		
		
	}

	aParams.resize(LightOptParams::MAX_PARAMS * mGpuLd->getLightCount()); aParams.setZero();
	uint32_t lightIndex = 0;
	for (const auto& pair : mLightParams) {
		
		if (pair.first->type == Ref::Type::Light) {
			const auto refLight = static_cast<RefLight*>(pair.first);
			if (pair.second[LightOptParams::POS_X]) {
				aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_X)) = refLight->position.x;
			}
			if (pair.second[LightOptParams::POS_Y]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_Y)) = static_cast<double>(refLight->position.y); }
			if (pair.second[LightOptParams::POS_Z]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_Z)) = static_cast<double>(refLight->position.z); }
		}
		
		const bool anyColor = pair.second[LightOptParams::COLOR_R] || pair.second[LightOptParams::COLOR_G] || pair.second[LightOptParams::COLOR_B];
		glm::vec3 color {0};
		float intensity{ 0.0f };
		if (pair.first->type == Ref::Type::Light) {
			const auto refLight = static_cast<RefLight*>(pair.first);
			color = refLight->light->getColor();
			intensity = refLight->light->getIntensity();
		} else if (pair.first->type == Ref::Type::Mesh) {
			const auto refMesh = static_cast<RefMesh*>(pair.first);
			color = refMesh->mesh->getMaterial()->getEmissionFactor();
			intensity = refMesh->mesh->getMaterial()->getEmissionStrength();
		} else throw std::runtime_error("");

		if (pair.second[LightOptParams::INTENSITY] && anyColor) { 
			const glm::vec3 colorWithIntensity = color * intensity;

			aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY)) = 1.0; 

			if (useQuadraticIntensityOpt) {
				if (pair.second[LightOptParams::COLOR_R]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)) = static_cast<double>(sqrt(colorWithIntensity.x * 2.0f)); }
				if (pair.second[LightOptParams::COLOR_G]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)) = static_cast<double>(sqrt(colorWithIntensity.y * 2.0f)); }
				if (pair.second[LightOptParams::COLOR_B]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)) = static_cast<double>(sqrt(colorWithIntensity.z * 2.0f)); }
			}
			else {
				if (pair.second[LightOptParams::COLOR_R]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)) = static_cast<double>(colorWithIntensity.x); }
				if (pair.second[LightOptParams::COLOR_G]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)) = static_cast<double>(colorWithIntensity.y); }
				if (pair.second[LightOptParams::COLOR_B]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)) = static_cast<double>(colorWithIntensity.z); }
			}

		}
		else if (pair.second[LightOptParams::INTENSITY] && !anyColor) { 
			if (useQuadraticIntensityOpt) { 
				aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY)) = static_cast<double>(sqrt(intensity * 2.0f));
			}
			else {
				aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY)) = static_cast<double>(intensity);
			}

		}
		else if (!(pair.second[LightOptParams::INTENSITY]) && anyColor) { 
			const float csum = color.x + color.y + color.z;
			color /= csum; 
			
			if (pair.first->type == Ref::Type::Light) static_cast<RefLight*>(pair.first)->light->setIntensity(intensity * csum);
			else if (pair.first->type == Ref::Type::Mesh) static_cast<RefMesh*>(pair.first)->mesh->getMaterial()->setEmissionStrength(intensity * csum);
			if (pair.second[LightOptParams::COLOR_R]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)) = static_cast<double>(color.x); }
			if (pair.second[LightOptParams::COLOR_G]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)) = static_cast<double>(color.y); }
			if (pair.second[LightOptParams::COLOR_B]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)) = static_cast<double>(color.z); }
		}

		
		if (pair.second[LightOptParams::ROT_X] || pair.second[LightOptParams::ROT_Y] || pair.second[LightOptParams::ROT_Z]) { 
			glm::vec3 scale, translation, skew; glm::quat rotation;  glm::vec4 persp;
			glm::decompose(pair.first->model_matrix, scale, rotation, translation, skew, persp);
			
			Eigen::AngleAxisf raa(Eigen::Quaternionf(rotation.w, rotation.x, rotation.y, rotation.z));
			Eigen::Vector3f rv = raa.axis() * raa.angle();

			if (pair.second[LightOptParams::ROT_X]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_X)) = static_cast<double>(rv.x()); }
			if (pair.second[LightOptParams::ROT_Y]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Y)) = static_cast<double>(rv.y()); }
			if (pair.second[LightOptParams::ROT_Z]) { aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Z)) = static_cast<double>(rv.z()); }
		}

		
		if (pair.first->type == Ref::Type::Light) {
			const auto refLight = static_cast<RefLight*>(pair.first);
			auto sl = std::dynamic_pointer_cast<SpotLight>(refLight->light);
			if (sl != nullptr && (pair.second[LightOptParams::CONE_INNER] || pair.second[LightOptParams::CONE_EDGE])) {
				coneAngleParameterization.setActiveParams(
					pair.second[LightOptParams::CONE_INNER],
					pair.second[LightOptParams::CONE_EDGE]
				);
				coneAngleParameterization.valuesToParams(
					aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_INNER)),
					aParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_EDGE)),
					sl->getInnerConeAngle(), sl->getOuterConeAngle()
				);
			}
		}
		lightIndex++;
	}

	const Eigen::VectorXd param = aParams;
	LightOptParams::reduceVectorToActiveParams(aParams, param, mLightParams);
}

void LightTraceOptimizer::lightDerivativesToVector(Eigen::VectorXd& aDerivParams )
{
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	std::vector<LightGrads> lightDerivsHost; lightDerivsHost.assign(mGpuLd->getLightCount(), LightGrads());
	mLightDerivativesBuffer.STC_DownloadData(&stc, lightDerivsHost.data(), mGpuLd->getLightCount() * sizeof(LightGrads));

	aDerivParams.resize(LightOptParams::MAX_PARAMS * mGpuLd->getLightCount());
	aDerivParams.setZero();

	Eigen::VectorXd params;
	LightOptParams::expandVectorToFullParams(params, mParams, mLightParams);

	uint32_t lightIndex = 0;
	for (const auto& pair : mLightParams) {
		
		if (pair.second[LightOptParams::POS_X]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_X)) = lightDerivsHost[lightIndex].dOdP[0];
		}
		if (pair.second[LightOptParams::POS_Y]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_Y)) = lightDerivsHost[lightIndex].dOdP[1]; }
		if (pair.second[LightOptParams::POS_Z]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_Z)) = lightDerivsHost[lightIndex].dOdP[2]; }

		
		bool anyColor = pair.second[LightOptParams::COLOR_R] || pair.second[LightOptParams::COLOR_G] || pair.second[LightOptParams::COLOR_B];
		if (pair.second[LightOptParams::INTENSITY] && anyColor) { 
			glm::vec3 colorWithIntensity;
			colorWithIntensity.x = params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)); 
			colorWithIntensity.y = params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G));
			colorWithIntensity.z = params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B));

			aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY)) = 0.0; 

			float intensity{ 0.0f };
			if (pair.first->type == Ref::Type::Light) {
				intensity = static_cast<RefLight*>(pair.first)->light->getIntensity();
			} else if (pair.first->type == Ref::Type::Mesh) {
				intensity = static_cast<RefMesh*>(pair.first)->mesh->getMaterial()->getEmissionStrength();
			} else throw std::runtime_error("");
			if (useQuadraticIntensityOpt) {
				if (pair.second[LightOptParams::COLOR_R]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)) = lightDerivsHost[lightIndex].dOdColor.x * colorWithIntensity.x / intensity; }
				if (pair.second[LightOptParams::COLOR_G]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)) = lightDerivsHost[lightIndex].dOdColor.y * colorWithIntensity.y / intensity; }
				if (pair.second[LightOptParams::COLOR_B]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)) = lightDerivsHost[lightIndex].dOdColor.z * colorWithIntensity.z / intensity; }
			}
			else {
				if (pair.second[LightOptParams::COLOR_R]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)) = lightDerivsHost[lightIndex].dOdColor.x / intensity; } 
				if (pair.second[LightOptParams::COLOR_G]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)) = lightDerivsHost[lightIndex].dOdColor.y / intensity; }
				if (pair.second[LightOptParams::COLOR_B]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)) = lightDerivsHost[lightIndex].dOdColor.z / intensity; }
			}

		}
		else if (pair.second[LightOptParams::INTENSITY] && !anyColor) { 
			if (useQuadraticIntensityOpt) {
				aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY)) = lightDerivsHost[lightIndex].dOdIntensity * params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY));
			}
			else {
				aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY)) = lightDerivsHost[lightIndex].dOdIntensity;
			}

		}
		else if (!(pair.second[LightOptParams::INTENSITY]) && anyColor) { 
			double& c1 = params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R));
			double& c2 = params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G));
			double& c3 = params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B));
			Eigen::Matrix3d dc; 
			dc(0, 0) = (-c1 * 1.0 / pow(c1 + c2 + c3, 2.0) + 1.0 / (c1 + c2 + c3));
			dc(0, 1) = (-c1 * 1.0 / pow(c1 + c2 + c3, 2.0));
			dc(0, 2) = (-c1 * 1.0 / pow(c1 + c2 + c3, 2.0));
			dc(1, 0) = (-c2 * 1.0 / pow(c1 + c2 + c3, 2.0));
			dc(1, 1) = (-c2 * 1.0 / pow(c1 + c2 + c3, 2.0) + 1.0 / (c1 + c2 + c3));
			dc(1, 2) = (-c2 * 1.0 / pow(c1 + c2 + c3, 2.0));
			dc(2, 0) = (-c3 * 1.0 / pow(c1 + c2 + c3, 2.0));
			dc(2, 1) = (-c3 * 1.0 / pow(c1 + c2 + c3, 2.0));
			dc(2, 2) = (-c3 * 1.0 / pow(c1 + c2 + c3, 2.0) + 1.0 / (c1 + c2 + c3));

			
			Eigen::Vector3d colorgrad = dc * Eigen::Map<Eigen::Vector3d>(&(lightDerivsHost[lightIndex].dOdColor.x));
			if (pair.second[LightOptParams::COLOR_R]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)) = colorgrad[0]; }
			if (pair.second[LightOptParams::COLOR_G]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)) = colorgrad[1]; }
			if (pair.second[LightOptParams::COLOR_B]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)) = colorgrad[2]; }
		}

		
		if (pair.first->type == Ref::Type::Light) {
			auto refLight = static_cast<RefLight*>(pair.first);
			if (pair.second[LightOptParams::ROT_X] || pair.second[LightOptParams::ROT_Y] || pair.second[LightOptParams::ROT_Z]) { 
				glm::vec3 scale, translation, skew; glm::quat rotation; glm::vec4 persp;
				glm::decompose(pair.first->model_matrix, scale, rotation, translation, skew, persp);
				
				Eigen::AngleAxisf raa(Eigen::Quaternionf(rotation.w, rotation.x, rotation.y, rotation.z));
				Eigen::Vector3f rv2 = raa.axis() * raa.angle();
				
				
				

				
				Eigen::Vector3f rv(
					params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_X)),
					params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Y)),
					params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Z))
				);
				if ((rv - rv2).squaredNorm() > 1e-3) {
					spdlog::warn("rotation vector inconsistent with parameters: computed ({},{},{}), params ({},{},{})", rv.x(), rv.y(), rv.z(), rv2.x(), rv2.y(), rv2.z());
				}

				Eigen::Matrix3d dndr;
				{	
					using r_t = double;
					Eigen::Matrix3d& dvdr = dndr; 
					const r_t& r1 = rv.x(), & r2 = rv.y(), & r3 = rv.z();
					const r_t  v1 = refLight->light->getDefaultDirection().x, v2 = refLight->light->getDefaultDirection().y, v3 = refLight->light->getDefaultDirection().z;
					if (rv.norm() > 1e-5) {
#include "codegen_dRdrv.h" 
					}
					else {
						dndr << 
							0.0, v3, -v2,
							-v3, 0.0, v1,
							v2, -v1, 0.0;
					}
				}
				Eigen::Matrix3d dtdr;
				{	
					using r_t = double;
					Eigen::Matrix3d& dvdr = dtdr; 
					const r_t& r1 = rv.x(), & r2 = rv.y(), & r3 = rv.z();
					const r_t  v1 = refLight->light->getDefaultTangent().x, v2 = refLight->light->getDefaultTangent().y, v3 = refLight->light->getDefaultTangent().z;
					if (rv.norm() > 1e-5f) {
#include "codegen_dRdrv.h" 
					}
					else {
						dtdr << 
							0.0, v3, -v2,
							-v3, 0.0, v1,
							v2, -v1, 0.0;
					}
				}
				Eigen::Vector3d dphidr = 
					Eigen::Map<Eigen::Vector3d>(&(lightDerivsHost[lightIndex].dOdN.x)).transpose() * dndr +
					Eigen::Map<Eigen::Vector3d>(&(lightDerivsHost[lightIndex].dOdT.x)).transpose() * dtdr;

				if (pair.second[LightOptParams::ROT_X]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_X)) = dphidr[0]; }
				if (pair.second[LightOptParams::ROT_Y]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Y)) = dphidr[1]; }
				if (pair.second[LightOptParams::ROT_Z]) { aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Z)) = dphidr[2]; }
			}

			
			auto sl = std::dynamic_pointer_cast<SpotLight>(refLight->light);
			if (sl != nullptr && (pair.second[LightOptParams::CONE_INNER] || pair.second[LightOptParams::CONE_EDGE])) {
				coneAngleParameterization.setActiveParams(
					pair.second[LightOptParams::CONE_INNER],
					pair.second[LightOptParams::CONE_EDGE]
				);
				coneAngleParameterization.derivativeChain(
					aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_INNER)),
					aDerivParams(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_EDGE)),
					lightDerivsHost[lightIndex].dOdIAngle,
					lightDerivsHost[lightIndex].dOdOAngle,
					params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_INNER)),
					params(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_EDGE))
				);
			}
		}
		lightIndex++;
	}
}

void LightTraceOptimizer::parameterVectorToLights(Eigen::VectorXd& aParams) {
	unsigned int texParamCount = 0;
	for(std::map<tamashii::Ref*, LightOptParams>::iterator it = mLightParams.begin(); it != mLightParams.end(); ++it){
		if(it->first->type == Ref::Type::Mesh){
			const RefMesh* refMesh = static_cast<RefMesh*>(it->first);
			if( mFirstEmissiveTexture == refMesh->mesh->getMaterial()->getEmissionTexture() && it->second[LightOptParams::EMISSIVE_TEXTURE]==true )
				texParamCount = ( mLightTextureDerivativesBuffer.getSize()/sizeof(double) );
		}
	}

	Eigen::VectorXd param;
	if((aParams.size()-texParamCount) == getActiveParameterCount()) {
		LightOptParams::expandVectorToFullParams(param, aParams, mLightParams);
	}
	else param = aParams.segment(0, aParams.size()-texParamCount);

	if (static_cast<size_t>(param.size()) != mGpuLd->getLightCount() * LightOptParams::MAX_PARAMS) {
		spdlog::warn("parameterVectorToLights wrong size (max {} active {} found {}) -- reloading from lights", mGpuLd->getLightCount() * LightOptParams::MAX_PARAMS, getActiveParameterCount(), param.size());
		Eigen::VectorXd currentLightParams;
		lightsToParameterVector(currentLightParams); 
		lightTextureToParameterVector(currentLightParams); 
		aParams = currentLightParams;
		spdlog::info("new param size is {}", aParams.size());
		LightOptParams::expandVectorToFullParams(param, currentLightParams, mLightParams);
	}

	uint32_t lightIndex = 0;
	for (const auto& pair : mLightParams) {
		
		if (pair.first->type == Ref::Type::Light) {
			const auto refLight = static_cast<RefLight*>(pair.first);
			if (pair.second[LightOptParams::POS_X]) { refLight->position.x = pair.first->model_matrix[3][0] = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_X));
			} 
			if (pair.second[LightOptParams::POS_Y]) { refLight->position.y = pair.first->model_matrix[3][1] = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_Y)); }
			if (pair.second[LightOptParams::POS_Z]) { refLight->position.z = pair.first->model_matrix[3][2] = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::POS_Z)); }
		}

		
		const bool anyColor = pair.second[LightOptParams::COLOR_R] || pair.second[LightOptParams::COLOR_G] || pair.second[LightOptParams::COLOR_B];
		glm::vec3 color {0};
		float intensity{ 0.0f };
		if (pair.first->type == Ref::Type::Light) {
			const auto refLight = static_cast<RefLight*>(pair.first);
			color = refLight->light->getColor();
			intensity = refLight->light->getIntensity();
		}
		else if (pair.first->type == Ref::Type::Mesh) {
			const auto refMesh = static_cast<RefMesh*>(pair.first);
			color = refMesh->mesh->getMaterial()->getEmissionFactor();
			intensity = refMesh->mesh->getMaterial()->getEmissionStrength();
		}
		else throw std::runtime_error("");

		if (pair.second[LightOptParams::INTENSITY] && anyColor) { 
			glm::vec3 colorWithIntensity = color * intensity;
			if (pair.second[LightOptParams::COLOR_R]) {
				if (useQuadraticIntensityOpt) {
					colorWithIntensity.x = 0.5f * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)) * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R));
				}
				else {
					colorWithIntensity.x = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R));
				}
			}
			if (pair.second[LightOptParams::COLOR_G]) {
				if (useQuadraticIntensityOpt) {
					colorWithIntensity.y = 0.5f * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)) * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G));
				}
				else {
					colorWithIntensity.y = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G));
				}
			}
			if (pair.second[LightOptParams::COLOR_B]) {
				if (useQuadraticIntensityOpt) {
					colorWithIntensity.z = 0.5f * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)) * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B));
				}
				else {
					colorWithIntensity.z = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B));
				}
			}
			const float cSum = colorWithIntensity.x + colorWithIntensity.y + colorWithIntensity.z;
			colorWithIntensity /= cSum;
			color = colorWithIntensity;
			intensity = cSum;

		}
		else if (pair.second[LightOptParams::INTENSITY] && !anyColor) { 
			if (useQuadraticIntensityOpt) {
				intensity = 0.5 * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY)) * param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY));
			}
			else {
				intensity = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::INTENSITY));
			}

		}
		else if (!(pair.second[LightOptParams::INTENSITY]) && anyColor) { 
			glm::vec3 newColor = color;
			if (pair.second[LightOptParams::COLOR_R]) { newColor.x = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_R)); }
			if (pair.second[LightOptParams::COLOR_G]) { newColor.y = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_G)); }
			if (pair.second[LightOptParams::COLOR_B]) { newColor.z = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::COLOR_B)); }
			const float csum = newColor.x + newColor.y + newColor.z;
			newColor /= csum; 
			color = newColor;
		}

		if (pair.first->type == Ref::Type::Light) {
			const auto refLight = static_cast<RefLight*>(pair.first);
			refLight->light->setColor(color);
			refLight->light->setIntensity(intensity);
		}
		else if (pair.first->type == Ref::Type::Mesh) {
			const auto refMesh = static_cast<RefMesh*>(pair.first);
			refMesh->mesh->getMaterial()->setEmissionFactor(color);
			refMesh->mesh->getMaterial()->setEmissionStrength(intensity);
		}
		else throw std::runtime_error("");


		
		if (pair.first->type == Ref::Type::Light) {
			auto refLight = static_cast<RefLight*>(pair.first);
			if (pair.second[LightOptParams::ROT_X] || pair.second[LightOptParams::ROT_Y] || pair.second[LightOptParams::ROT_Z]) { 
				glm::vec3 scale, translation, skew; glm::quat rotation;  glm::vec4 persp;
				glm::decompose(pair.first->model_matrix, scale, rotation, translation, skew, persp);
				
				
				
				Eigen::Vector3f rv; rv.setZero(); 

				
				if (pair.second[LightOptParams::ROT_X]) {
					rv.x() = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_X));
				}
				if (pair.second[LightOptParams::ROT_Y]) { rv.y() = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Y)); }
				if (pair.second[LightOptParams::ROT_Z]) { rv.z() = param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::ROT_Z)); }

				if (rv.norm() > 1.5 * M_PI) {
					
					rv -= rv.normalized() * 2.0 * M_PI;
					
					
				}

				
				glm::mat4 mdlMatrix = glm::mat4(1.0f);
				mdlMatrix = glm::translate(mdlMatrix, translation);

				if (rv.norm() > 1e-5f) {
					
					Eigen::AngleAxisf raa;
					raa.angle() = rv.norm();
					raa.axis() = rv / raa.angle(); 
					auto rq = Eigen::Quaternionf(raa);
					rotation.w = rq.w(); rotation.x = rq.x(); rotation.y = rq.y(); rotation.z = rq.z();
					mdlMatrix = mdlMatrix * glm::toMat4(rotation);
				}
				else {
					
					glm::mat4 linRot( 
						1.0, -rv.z(), rv.y(), 0.0,
						rv.z(), 1.0, -rv.x(), 0.0,
						-rv.y(), rv.x(), 1.0, 0.0,
						0.0, 0.0, 0.0, 1.0
					);
					mdlMatrix = mdlMatrix * linRot;
				}

				mdlMatrix = glm::scale(mdlMatrix, scale);
				

				pair.first->model_matrix = mdlMatrix;
				refLight->direction = glm::normalize(pair.first->model_matrix * refLight->light->getDefaultDirection());
			}

			
			auto sl = std::dynamic_pointer_cast<SpotLight>(refLight->light);
			if (sl != nullptr && (pair.second[LightOptParams::CONE_INNER] || pair.second[LightOptParams::CONE_EDGE])) {
				coneAngleParameterization.setActiveParams(
					pair.second[LightOptParams::CONE_INNER],
					pair.second[LightOptParams::CONE_EDGE]
				);
				double inner = sl->getInnerConeAngle(), outer = sl->getOuterConeAngle();
				coneAngleParameterization.paramsToValues(
					param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_INNER)),
					param(LightOptParams::getParameterIndex(lightIndex, LightOptParams::CONE_EDGE)),
					inner, outer
				);
				sl->setCone(inner, outer);
			}
		}
		lightIndex++;
	}

	mRoot.device.waitIdle();
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	mGpuLd->update(&stc, mLights, mGpuTd, mModels, mGpuBlas);
	
	mGpuMd->update(&stc, *mMaterials, mGpuTd); 
}




unsigned char clampToColor(double c) {
	c *= 255.0;
	if( c<0.0 ) c=0.0;
	else if( c>255.0 ) c=255.0;
	return (unsigned char) c;
}

void LightTraceOptimizer::lightTextureToParameterVector(Eigen::VectorXd& aParams){
	if( mFirstEmissiveTexture==NULL ) return; 
	bool textureActive = false;
	for(std::map<tamashii::Ref*, LightOptParams>::iterator it = mLightParams.begin(); it != mLightParams.end(); ++it){
		if(it->first->type == Ref::Type::Mesh){
			const RefMesh* refMesh = static_cast<RefMesh*>(it->first);
			if( mFirstEmissiveTexture == refMesh->mesh->getMaterial()->getEmissionTexture() && it->second[LightOptParams::EMISSIVE_TEXTURE]==true )
				textureActive = true;
		}
	}
	if(!textureActive) return;

	if (mFirstEmissiveTexture->image->getFormat() != Image::Format::RGBA8_SRGB) {
		spdlog::error("image format not supported in LightTraceOptimizer::lightTextureToParameterVector");
		return;
	}
	unsigned int bytesPerPixel = mFirstEmissiveTexture->image->getPixelSizeInBytes();
	unsigned char* imageData = (unsigned char*)( mFirstEmissiveTexture->image->getData() );

	Eigen::VectorXd lightTexParams; lightTexParams.resize( mLightTextureDerivativesBuffer.getSize()/sizeof(double) ); lightTexParams.setZero();

	if( lightTexParams.size() != 3*mFirstEmissiveTexture->image->getWidth()*mFirstEmissiveTexture->image->getHeight() ){
		spdlog::error("buffer size mismatch for emissive texture in LightTraceOptimizer::lightTextureToParameterVector");
		return;
	}

	for(int i = 0; i < mFirstEmissiveTexture->image->getWidth(); ++i){
		for(int j = 0; j < mFirstEmissiveTexture->image->getHeight(); ++j){
			unsigned int idx = i + mFirstEmissiveTexture->image->getWidth() * j;
			unsigned int offset = bytesPerPixel * idx;
			lightTexParams[idx*3  ] = (double) imageData[offset  ]/255.0;
			lightTexParams[idx*3+1] = (double) imageData[offset+1]/255.0;
			lightTexParams[idx*3+2] = (double) imageData[offset+2]/255.0;
		}
	}

	
	

	int regularParamCount = aParams.size();
	aParams.conservativeResize( regularParamCount + lightTexParams.size() );
	aParams.segment( regularParamCount, lightTexParams.size() ) = lightTexParams;
}

void LightTraceOptimizer::parameterVectorToLightTexture(Eigen::VectorXd& aParams){
	if( mFirstEmissiveTexture==NULL ) return; 
	bool textureActive = false;
	for(std::map<tamashii::Ref*, LightOptParams>::iterator it = mLightParams.begin(); it != mLightParams.end(); ++it){
		if(it->first->type == Ref::Type::Mesh){
			const RefMesh* refMesh = static_cast<RefMesh*>(it->first);
			if( mFirstEmissiveTexture == refMesh->mesh->getMaterial()->getEmissionTexture() && it->second[LightOptParams::EMISSIVE_TEXTURE]==true )
				textureActive = true;
		}
	}
	if(!textureActive) return;

	if( mFirstEmissiveTexture->image->getFormat() != Image::Format::RGBA8_SRGB ){
		spdlog::error("image format not supported in LightTraceOptimizer::parameterVectorToLightTexture");
		return;
	}

	unsigned int bytesPerPixel = mFirstEmissiveTexture->image->getPixelSizeInBytes();
	unsigned char* imageData = (unsigned char*)( mFirstEmissiveTexture->image->getData() );

	Eigen::VectorXd lightTexParams; lightTexParams.resize( mLightTextureDerivativesBuffer.getSize()/sizeof(double) ); lightTexParams.setZero();

	if( lightTexParams.size() != 3*mFirstEmissiveTexture->image->getWidth()*mFirstEmissiveTexture->image->getHeight() ){
		spdlog::error("buffer size mismatch for emissive texture in LightTraceOptimizer::parameterVectorToLightTexture");
		return;
	}
	if( aParams.size() < lightTexParams.size() ){
		spdlog::error("parameter vector size mismatch for emissive texture in LightTraceOptimizer::parameterVectorToLightTexture");
		return;
	}

	lightTexParams = aParams.segment( aParams.size() - lightTexParams.size(), lightTexParams.size() );

	for(int i = 0; i < mFirstEmissiveTexture->image->getWidth(); ++i){
		for(int j = 0; j < mFirstEmissiveTexture->image->getHeight(); ++j){
			int idx = i + mFirstEmissiveTexture->image->getWidth() * j;
			unsigned int offset = bytesPerPixel * idx;
			imageData[offset  ] = clampToColor( lightTexParams[idx*3  ] );
			imageData[offset+1] = clampToColor( lightTexParams[idx*3+1] );
			imageData[offset+2] = clampToColor( lightTexParams[idx*3+2] );
			
			
			lightTexParams[idx*3  ] = (double) imageData[offset  ]/255.0;
			lightTexParams[idx*3+1] = (double) imageData[offset+1]/255.0;
			lightTexParams[idx*3+2] = (double) imageData[offset+2]/255.0;

		}
	}

	
	mRoot.device.waitIdle();
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	mGpuTd->update(&stc, *mImages, *mTextures);
	
	
	

}

double LightTraceOptimizer::lightTextureDerivativesToVector(Eigen::VectorXd& aDerivParams){
	if( mFirstEmissiveTexture==NULL ) return 0.0; 
	bool textureActive = false;
	for(std::map<tamashii::Ref*, LightOptParams>::iterator it = mLightParams.begin(); it != mLightParams.end(); ++it){
		if(it->first->type == Ref::Type::Mesh){
			const RefMesh* refMesh = static_cast<RefMesh*>(it->first);
			if( mFirstEmissiveTexture == refMesh->mesh->getMaterial()->getEmissionTexture() && it->second[LightOptParams::EMISSIVE_TEXTURE]==true )
				textureActive = true;
		}
	}
	if(!textureActive) return 0.0;

	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	
	Eigen::VectorXd lightTexDerivsHost; lightTexDerivsHost.resize( mLightTextureDerivativesBuffer.getSize()/sizeof(double) ); lightTexDerivsHost.setZero();
	mLightTextureDerivativesBuffer.STC_DownloadData(&stc, lightTexDerivsHost.data(), mLightTextureDerivativesBuffer.getSize());
	

	
	
	

	


	int regularParamCount = aDerivParams.size();
	aDerivParams.conservativeResize( regularParamCount + lightTexDerivsHost.size() );
	aDerivParams.segment( regularParamCount, lightTexDerivsHost.size() ) = lightTexDerivsHost;

	return 0.0;
}
