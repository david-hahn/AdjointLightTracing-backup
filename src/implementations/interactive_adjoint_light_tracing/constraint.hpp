#pragma once

#include <tamashii/core/scene/ref_entities.hpp>
#include "parameter.hpp"

#include <Eigen/Eigen>
#include <set>

class LightConstraint{
public:
	LightConstraint() : mPenaltyFactor(1.0), mIsActive(true) {}
	virtual ~LightConstraint() = default;
	void setPenaltyFactor(const double aFactor = 1.0) { mPenaltyFactor = aFactor; }
	void setActive(const bool aActive = true) { mIsActive = aActive; }
	void addLight(tamashii::RefLight* aRefLight) { mLights.insert(aRefLight); }
	void removeLight(tamashii::RefLight* aRefLight) { mLights.erase(aRefLight); }
	/*
	 * Evaluate the constraint function.
	 * The result should be 0 iff the constraint is satisfied, and >0 otherwise.
	 * The constraint value should be a distance to the closest constraint satisfying configuration and provide a gradient accordingly.
	 * The result (and the gradient) should be multiplied by the penaltyFactor, for use in penalty-based soft constraint formulations.
	 * If the constraint is inactive (isActive==false) the result should always be 0 (with zero gradient).
	 * The size of the gradient should be lights.size() * LightOptParams::MAX_PARAMS, and the order of entries per light should follow LightOptParams::PARAMS
	 */
	virtual double evalAndAddToGradient(Eigen::VectorXd& aGradient) = 0;
protected:
	std::set<tamashii::RefLight*> mLights;
	double mPenaltyFactor;
	bool mIsActive;
};

class LightsInAABBConstraint final : public LightConstraint{
public:
	LightsInAABBConstraint(const double aXmin, const double aXmax, const double aYmin, const double aYmax, const double aZmin, const double aZmax) : LightConstraint(),
		mXmin(aXmin), mXmax(aXmax), mYmin(aYmin), mYmax(aYmax), mZmin(aZmin), mZmax(aZmax) {}
	
	double evalAndAddToGradient(Eigen::VectorXd& aGradient) override
	{
		double f = 0.0;
		if( mIsActive ){
			for(const tamashii::RefLight* refLight : mLights)
			{
				const auto idx = static_cast<Eigen::Index>(refLight->ref_light_index);
				const glm::vec3& p = refLight->position;
				Eigen::Vector3d d; d.setZero();

				if (static_cast<double>(p.x) < mXmin) d[0] = static_cast<double>(p.x) - mXmin;
				else if (static_cast<double>(p.x) > mXmax) d[0] = static_cast<double>(p.x) - mXmax;

				if (static_cast<double>(p.y) < mYmin) d[1] = static_cast<double>(p.y) - mYmin;
				else if (static_cast<double>(p.y) > mYmax) d[1] = static_cast<double>(p.y) - mYmax;

				if (static_cast<double>(p.z) < mZmin) d[2] = static_cast<double>(p.z) - mZmin;
				else if (static_cast<double>(p.z) > mZmax) d[2] = static_cast<double>(p.z) - mZmax;

				f += 0.5 * mPenaltyFactor * d.squaredNorm();
				aGradient.segment(idx * LightOptParams::MAX_PARAMS + LightOptParams::POS_X, 3) += mPenaltyFactor * d;
			}
		}
		return f;
	}
private:
	double mXmin, mXmax, mYmin, mYmax, mZmin, mZmax;
};

class LightsIntensityPenalty final : public LightConstraint{
public:
	LightsIntensityPenalty(const double penaltyFactor_){ mPenaltyFactor = penaltyFactor_; }
	
	double evalAndAddToGradient(Eigen::VectorXd& aGradient) override
	{
		double f = 0.0;
		if( mIsActive ){
			for(const tamashii::RefLight* refLight : mLights)
			{
				const auto idx = static_cast<Eigen::Index>(refLight->ref_light_index);
				double intensity = refLight->light->getIntensity();
				f += mPenaltyFactor * 0.5*intensity*intensity; 
				
				
#ifdef IALT_USE_QUADRATIC_INTENSITY_OPT
				
				
				
				
				aGradient(idx * LightOptParams::MAX_PARAMS + LightOptParams::INTENSITY) += mPenaltyFactor*intensity*sqrt(2.0*intensity);
#else
				aGradient(idx * LightOptParams::MAX_PARAMS + LightOptParams::INTENSITY) += mPenaltyFactor*intensity;
#endif
			}
		}
		return f;
	}
};
