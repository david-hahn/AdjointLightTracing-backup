#pragma once
#include "LBFGSpp/LBFGS.h"
#include "cmaes/CMAMinimizer.h"
#include "light_trace_opti.hpp"

template <typename VectorType> 
class OptimWrapperBase{
public:
    using			Real = typename VectorType::Scalar;
	
	struct OptimizationResult {
		Real bestObjectiveValue;
		Real lastPhi;
	};

					OptimWrapperBase(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) :
						mBestObjectiveValue(DBL_MAX), mEvals(0), mIters(0), mMaxIters(100),
						mSim(aSim), mRadianceBufferOut(aRadianceBufferCopy) {}
	virtual			~OptimWrapperBase() = default;

    Real			operator()(VectorType& aParams, VectorType& aGrads)
					{
						mSim->forward(aParams, mRadianceBufferOut);
						Real phi = mSim->backward(aGrads);
				        ++mEvals;
				        if( phi < mBestObjectiveValue ){
							mSim->addCurrentStateToHistory(aParams);
				            mBestObjectiveValue = phi;
				            mBestRunParameters = aParams;
				            mBestRunGradient = aGrads;
				        }
						if (!mSim->optimizationRunning()) aGrads.setZero();
				        return phi;
				    }
	virtual OptimizationResult runOptimization(VectorType& aParams) = 0;

	Real			mBestObjectiveValue;
	VectorType		mBestRunParameters, mBestRunGradient;
	unsigned int	mEvals, mIters, mMaxIters;
protected:
	LightTraceOptimizer*mSim;
	rvk::Buffer*	mRadianceBufferOut;
};

template <typename VectorType> 
class LBFGSppWrapper final : public OptimWrapperBase<VectorType>{
public:
    using Real = typename VectorType::Scalar;
	
    LBFGSpp::LBFGSParam<Real> mOptimOptions;
	LBFGSpp::LBFGSParam<Real> defaultLBFGSoptions() {
						LBFGSpp::LBFGSParam<Real> optimOptions;
						optimOptions.linesearch = LBFGSpp::LBFGS_LINESEARCH_BACKTRACKING_WOLFE;
						optimOptions.wolfe = 1.0-1e-6;
						optimOptions.m = 10;
						optimOptions.epsilon = 1e-6;
						optimOptions.delta = 1e-6; /*relative change of objective function to previous iteration below which we give up*/
						optimOptions.past = 1; /*0 == off, 1 == compare to previous iteration to detect if we got stuck ...*/
						optimOptions.max_linesearch = 15;
						optimOptions.removeOldestOnCurvatureFailure = false;
						return optimOptions;
					}
					LBFGSppWrapper(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) :
						OptimWrapperBase<VectorType>(aSim, aRadianceBufferCopy), mOptimOptions(defaultLBFGSoptions()) {}
	
	OptimWrapperBase<VectorType>::OptimizationResult runOptimization(VectorType& aParams) override
				    {
				        mOptimOptions.max_iterations = this->mMaxIters;
						LBFGSpp::LBFGSSolver<Real> solver(mOptimOptions); 
						Real phi;

				        this->mIters = solver.minimize(*this, aParams, phi);

				        aParams = this->mBestRunParameters;
						return { .bestObjectiveValue = this->mBestObjectiveValue, .lastPhi = phi };
					}
};

template <typename VectorType> 
class SimpleGradientDescentWrapper final : public OptimWrapperBase<VectorType>{
public:
    using			Real = typename VectorType::Scalar;
	Real			mStepSize, mGradTolerance;

					SimpleGradientDescentWrapper(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) :
						OptimWrapperBase<VectorType>(aSim,aRadianceBufferCopy), mStepSize(Real(1.0)), mGradTolerance(Real(1e-6)) {}
	OptimWrapperBase<VectorType>::OptimizationResult runOptimization(VectorType& aParams) override
					{
						Real phi;
						this->mIters = 0; this->mEvals = 0;
						VectorType dp;
						phi = (*this)(aParams,dp);

						while ((dp.squaredNorm() > mGradTolerance * mGradTolerance) && (this->mIters <= this->mMaxIters)) {
							++this->mIters;
							aParams -= mStepSize * dp;
							phi = (*this)(aParams, dp);
						}

						aParams = this->mBestRunParameters;
						return { .bestObjectiveValue = this->mBestObjectiveValue, .lastPhi = phi };
					}
};



template <typename VectorType> 
class AdamWrapper final : public OptimWrapperBase<VectorType>{
public:
    using			Real = typename VectorType::Scalar;
	Real			mStepSize, mGradTolerance;
	Real			mBeta1, mBeta2, mEps;

					AdamWrapper(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) : OptimWrapperBase<VectorType>(aSim, aRadianceBufferCopy),
						mStepSize(Real(1.0)), mGradTolerance(Real(1e-6)), mBeta1(Real(0.9)), mBeta2(Real(0.999)), mEps(Real(1e-8)) {}
	OptimWrapperBase<VectorType>::OptimizationResult runOptimization(VectorType& aParams) override
					{
						Real phi, beta1Exp = mBeta1, beta2Exp = mBeta2, m_bias, v_bias;
						this->mIters = 0; this->mEvals = 0;
						VectorType dp, m,v;
						phi = (*this)(aParams,dp);

						m.resize(dp.size()); m.setZero();
						v.resize(dp.size()); v.setZero();

						while ((dp.squaredNorm() > mGradTolerance * mGradTolerance) && (this->mIters <= this->mMaxIters)) {
							++this->mIters;

							m = mBeta1 * m + (1.0 - mBeta1) * dp;
							v = mBeta2 * v + (1.0 - mBeta2) * (dp.array().square().matrix());

							m_bias = (1.0 - beta1Exp); beta1Exp *= mBeta1;
							v_bias = (1.0 - beta2Exp); beta2Exp *= mBeta2;

							aParams -= mStepSize * (sqrt(v_bias) / m_bias) * (m.array() / (v.array().sqrt() + mEps)).matrix();
							phi = (*this)(aParams, dp);
						}

				        aParams = this->mBestRunParameters;
						return { .bestObjectiveValue = this->mBestObjectiveValue, .lastPhi = phi };
					}
};


/*
FiniteDifferenceCheckWrapper is NOT an optimization method.
It simply computes a finite difference approximation of the objective gradient around the given parameters.
It returns 
- the norm of the difference between the FD-gradient and the gradient supplied by the simulation in bestObjectiveValue
- the finite difference result in bestRunGradient
- and the simulation supplied gradient in bestRunParameters
*/
template <typename VectorType> 
class FiniteDifferenceForwardCheckWrapper final : public OptimWrapperBase<VectorType> {
public:
    using			Real = typename VectorType::Scalar;
	Real			mStepSize;

					FiniteDifferenceForwardCheckWrapper(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) :
						OptimWrapperBase<VectorType>(aSim,aRadianceBufferCopy), mStepSize(Real(1e-4)) {}
	OptimWrapperBase<VectorType>::OptimizationResult runOptimization(VectorType& aParams) override {
						Real phi, phiFd, tmp;
						this->mIters = 0; this->mEvals = 0;
						VectorType dp, dpFd, unused;

						dpFd.resize(aParams.size());

						for (int k = 0; k < aParams.size(); ++k) {
							++this->mIters;
							tmp = aParams[k];
							aParams[k] += mStepSize;

							phiFd = (*this)(aParams, unused);
							dpFd[k] = phiFd;

							aParams[k] = tmp;
						}

						phi = (*this)(aParams, dp);
						dpFd.array() = (dpFd.array() - phi) / mStepSize;

						this->mBestObjectiveValue = (dpFd - dp).norm();
						this->mBestRunGradient = dpFd;
						this->mBestRunParameters = dp;

						return { .bestObjectiveValue = phi, .lastPhi = phi };
					}
};

template <typename VectorType> 
class FiniteDifferenceCentralCheckWrapper final : public OptimWrapperBase<VectorType> {
public:
	using			Real = typename VectorType::Scalar;
	Real			mStepSize;

					FiniteDifferenceCentralCheckWrapper(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) :
						OptimWrapperBase<VectorType>(aSim, aRadianceBufferCopy), mStepSize(Real(1e-4)) {}
	OptimWrapperBase<VectorType>::OptimizationResult runOptimization(VectorType& aParams) override {
						Real phi, phiFd, phiBd, tmp;
						this->mIters = 0; this->mEvals = 0;
						VectorType dp, dpFd, dpBd, unused;

						dpFd.resize(aParams.size());
						dpBd.resize(aParams.size());

						for (int k = 0; k < aParams.size(); ++k) {
							++this->mIters;
							tmp = aParams[k];

							aParams[k] += mStepSize;
							phiFd = (*this)(aParams, unused);
							dpFd[k] = phiFd;

							aParams[k] = tmp;
							aParams[k] -= mStepSize;
							phiBd = (*this)(aParams, unused);
							dpBd[k] = phiBd;

							aParams[k] = tmp;
						}

						phi = (*this)(aParams, dp);
						dpFd.array() = (dpFd.array() - dpBd.array()) / (2.0 * mStepSize);

						this->mBestObjectiveValue = (dpFd - dp).norm();
						this->mBestRunGradient = dpFd;
						this->mBestRunParameters = dp;

						return { .bestObjectiveValue = phi, .lastPhi = phi };
	}
};

template <typename VectorType> 
class FiniteDifferenceCentralAdamWrapper final : public OptimWrapperBase<VectorType> {
public:
	using			Real = typename VectorType::Scalar;
	Real			mStepSize, mGradTolerance;
	Real			mBeta1, mBeta2, mEps;

	FiniteDifferenceCentralAdamWrapper(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) : OptimWrapperBase<VectorType>(aSim, aRadianceBufferCopy),
		mStepSize(Real(1.0)), mGradTolerance(Real(1e-6)), mBeta1(Real(0.9)), mBeta2(Real(0.999)), mEps(Real(1e-8)) {}
	OptimWrapperBase<VectorType>::OptimizationResult runOptimization(VectorType& aParams) override
	{
		Real phi, beta1Exp = mBeta1, beta2Exp = mBeta2, m_bias, v_bias;
		this->mIters = 0; this->mEvals = 0;
		VectorType dp, m, v, unused;
		

		float h = 0.2f;
		{
			Real phiFd, phiBd, tmp;
			VectorType dpFd, dpBd;

			dpFd.resize(aParams.size());
			dpBd.resize(aParams.size());

			for (int k = 0; k < aParams.size(); ++k) {
				tmp = aParams[k];

				aParams[k] += h;
				phiFd = (*this)(aParams, unused);
				dpFd[k] = phiFd;

				aParams[k] = tmp;
				aParams[k] -= h;
				phiBd = (*this)(aParams, unused);
				dpBd[k] = phiBd;

				aParams[k] = tmp;
			}

			phi = (*this)(aParams, unused);
			dp.array() = (dpFd.array() - dpBd.array()) / (2.0 * h);
		}

		m.resize(dp.size()); m.setZero();
		v.resize(dp.size()); v.setZero();

		while ((dp.squaredNorm() > mGradTolerance * mGradTolerance) && (unused.squaredNorm() > mGradTolerance * mGradTolerance) && (this->mIters <= this->mMaxIters)) {
			++this->mIters;

			m = mBeta1 * m + (1.0 - mBeta1) * dp;
			v = mBeta2 * v + (1.0 - mBeta2) * (dp.array().square().matrix());

			m_bias = (1.0 - beta1Exp); beta1Exp *= mBeta1;
			v_bias = (1.0 - beta2Exp); beta2Exp *= mBeta2;

			aParams -= mStepSize * (sqrt(v_bias) / m_bias) * (m.array() / (v.array().sqrt() + mEps)).matrix();
			{
				Real phiFd, phiBd, tmp;
				VectorType dpFd, dpBd;

				dpFd.resize(aParams.size());
				dpBd.resize(aParams.size());

				for (int k = 0; k < aParams.size(); ++k) {
					tmp = aParams[k];

					aParams[k] += h;
					phiFd = (*this)(aParams, unused);
					dpFd[k] = phiFd;

					aParams[k] = tmp;
					aParams[k] -= h;
					phiBd = (*this)(aParams, unused);
					dpBd[k] = phiBd;

					aParams[k] = tmp;
				}

				phi = (*this)(aParams, unused);
				dp.array() = (dpFd.array() - dpBd.array()) / (2.0 * h);
			}
		}

		aParams = this->mBestRunParameters;
		return { .bestObjectiveValue = this->mBestObjectiveValue, .lastPhi = phi };
	}
};


class CMAWrapper final : public OptimWrapperBase<Eigen::VectorXd>{
public:
	using VectorType = Eigen::VectorXd;
    using			Real = typename VectorType::Scalar;
	Real			mInitStdDev;

					CMAWrapper(LightTraceOptimizer* aSim, rvk::Buffer* aRadianceBufferCopy = nullptr) :
						OptimWrapperBase<VectorType>(aSim,aRadianceBufferCopy), mInitStdDev(Real(1.0)) {}
	Real evaluate(VectorType& p){
		VectorType unused;
		return ((*this)(p,unused));
	}
	OptimWrapperBase<VectorType>::OptimizationResult runOptimization(VectorType& aParams) override
					{
						this->mIters = 0; this->mEvals = 0;
						Real phi=0.0;

						CMAMinimizer cma(/*maxIterations*/ mMaxIters, /*populationSize*/ 16, /*initialStdDev*/ mInitStdDev, /*epsilon*/ 1e-5, /*delta*/ 1e-5);
						cma.minimize(this, aParams, phi);

						aParams = this->mBestRunParameters;
						return { .bestObjectiveValue = this->mBestObjectiveValue, .lastPhi = phi };
					}
};
