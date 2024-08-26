#pragma once

#include <tamashii/core/scene/ref_entities.hpp>

#include <Eigen/Eigen>

class LightOptParams{
public:
	enum PARAMS { POS_X, POS_Y, POS_Z, INTENSITY, ROT_X, ROT_Y, ROT_Z, CONE_INNER, EMISSIVE_TEXTURE = CONE_INNER, CONE_EDGE, COLOR_R, COLOR_G, COLOR_B   /*add one more, so we can easily access the number of parameters*/, MAX_PARAMS };
	

	LightOptParams();

	void reset();
	void setDefault();
	void setAll();
	void setRotations();
	void setPositions();

	bool& operator[](PARAMS param) { return mOptimize[param]; }
	bool operator[](PARAMS param) const { return mOptimize[param]; }

	static const char* name(PARAMS);
	static uint32_t getActiveParameterCount(const std::map<tamashii::Ref*, LightOptParams>& aLightParams);
	static uint32_t	getParameterIndex(uint32_t aLightIdx, uint32_t aParamIdx);
	static void reduceVectorToActiveParams(Eigen::VectorXd& aReducedParams, const Eigen::VectorXd& aParams, std::map<tamashii::Ref*, LightOptParams>& aLightParams);
	static void	expandVectorToFullParams(Eigen::VectorXd& aFullParams, const Eigen::VectorXd& aParams, std::map<tamashii::Ref*, LightOptParams>& aLightParams);

	bool mOptimize[MAX_PARAMS]; 
	static const char* names[MAX_PARAMS];
};

class ConeAngleTanhParameterization {
public:
	ConeAngleTanhParameterization(const double aP1Scale = 1e-2, const bool aP1Active = true, const bool aP2Active = false) :
		mP1Scale(aP1Scale), mP2Scale(aP1Scale), mP1Active(aP1Active), mP2Active(aP2Active), mInnerMax(1.5),
		mEdgeMin(3.141592653589793 / 2.0 - mInnerMax) /*[rad] = ~ 86 deg*/ {}

	void setActiveParams(const bool aP1Active = true, const bool aP2Active = false);
	void paramsToValues(const double aP1, const double aP2, double& aInner, double& aOuter) const;
	void derivativeChain(double& aDfDp1, double& aDfDp2, const double aDfDi, const double aDfDo, const double aP1, const double aP2) const;
	void valuesToParams(double& aP1, double& aP2, const double aInner, const double aOuter) const;

private:
	double mP1Scale, mP2Scale;
	bool mP1Active, mP2Active;
	double mInnerMax, mEdgeMin;
};
