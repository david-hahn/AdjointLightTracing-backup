#include "parameter.hpp"

T_USE_NAMESPACE

const char* LightOptParams::names[LightOptParams::MAX_PARAMS] = {
	"posX",
	"posY",
	"posZ",
	"intensity",
	"rotX",
	"rotY",
	"rotZ",
	"coneInner",
	"coneEdge",
	"colorR",
	"colorG",
	"colorB"
};

LightOptParams::LightOptParams()
	: mOptimize() {
	setDefault();
}

void LightOptParams::reset()
{
	for (bool& k : mOptimize) {
		k = false;
	}
}

void LightOptParams::setDefault()
{
	reset();
	
}

void LightOptParams::setAll()
{
	for (bool& k : mOptimize) {
		k = true;
	}
}

void LightOptParams::setRotations()
{
	mOptimize[ROT_X] = true;
	mOptimize[ROT_Y] = true;
	mOptimize[ROT_Z] = true;
}

void LightOptParams::setPositions()
{
	mOptimize[POS_X] = true;
	mOptimize[POS_Y] = true;
	mOptimize[POS_Z] = true;
}

const char* LightOptParams::name(PARAMS param)
{
	if (param < 0 || param >= MAX_PARAMS) {
		throw std::runtime_error{ "Unknown optimization parameter name" };
	}

	return names[param];
}

uint32_t LightOptParams::getActiveParameterCount(const std::map<tamashii::Ref*, LightOptParams>& aLightParams) {
	uint32_t n = 0;
	for (const auto& pair : aLightParams)
	{
		for (const bool k : pair.second.mOptimize) if (k) ++n;
	}
	return n;
}

uint32_t LightOptParams::getParameterIndex(const uint32_t aLightIdx, const uint32_t aParamIdx) {
	return aLightIdx * MAX_PARAMS + aParamIdx;
}

void LightOptParams::reduceVectorToActiveParams(Eigen::VectorXd& aReducedParams, const Eigen::VectorXd& aParams, std::map<tamashii::Ref*, LightOptParams>& aLightParams) {
	aReducedParams.resize(getActiveParameterCount(aLightParams));
	aReducedParams.setZero();

	std::map<unsigned int, tamashii::Ref*> orderByIndex;
	for (const auto& pair : aLightParams) {
		if (pair.first->type == Ref::Type::Light) {
			const auto l = static_cast<RefLight*>(pair.first);
			orderByIndex[l->ref_light_index] = l;
		}
	}
	
	const uint32_t lights = orderByIndex.size();
	uint32_t models = 0;
	for (const auto& pair : aLightParams) {
		if (pair.first->type == Ref::Type::Mesh) {
			orderByIndex[lights + models] = pair.first;
			models++;
		}
	}

	uint32_t j = 0;
	for (auto it = orderByIndex.begin(); it != orderByIndex.end(); ++it) {
		for (uint32_t k = 0; k < MAX_PARAMS; ++k) {
			if (aLightParams[(it->second)].mOptimize[k]) aReducedParams[j++] = aParams[getParameterIndex(it->first, k)];
		}
	}
}

void LightOptParams::expandVectorToFullParams(Eigen::VectorXd& aFullParams, const Eigen::VectorXd& aParams, std::map<tamashii::Ref*, LightOptParams>& aLightParams) {
	aFullParams.resize(MAX_PARAMS * aLightParams.size()); aFullParams.setZero();

	std::map<unsigned int, tamashii::Ref*> orderByIndex;
	for (const auto& pair : aLightParams) {
		if (pair.first->type == Ref::Type::Light) {
			const auto l = static_cast<RefLight*>(pair.first);
			orderByIndex[l->ref_light_index] = l;
		}
	}
	const uint32_t lights = orderByIndex.size();
	uint32_t models = 0;
	for (const auto& pair : aLightParams) {
		if (pair.first->type == Ref::Type::Mesh) {
			orderByIndex[lights + models] = pair.first;
			models++;
		}
	}

	uint32_t j = 0;
	for (auto it = orderByIndex.begin(); it != orderByIndex.end(); ++it) {
		for (uint32_t k = 0; k < MAX_PARAMS; ++k) {
			Ref* ptr = it->second;
			bool opti = aLightParams[ptr].mOptimize[k];
			if (aLightParams[ptr].mOptimize[k]) aFullParams[getParameterIndex(it->first, k)] = aParams[j++];
		}
	}
}

void ConeAngleTanhParameterization::setActiveParams(const bool aP1Active, const bool aP2Active) {
	mP1Active = aP1Active; mP2Active = aP2Active;
	if (!mP1Active && mP2Active) {
		mP2Active = false; 
		printf("ConeAngleTanhParameterization::setActiveParams: edge/outer angle standalone optimization not supported - ignoring parameter settings\n");
	}
}


void ConeAngleTanhParameterization::paramsToValues(const double aP1, const double aP2, double& aInner, double& aOuter) const
{
	if (mP1Active) {
		aInner = mInnerMax * (tanh(aP1 * mP1Scale) * (1.0 / 2.0) + 1.0 / 2.0);
		if (mP2Active) aOuter = -mInnerMax + 3.141592653589793 * (1.0 / 2.0) + mInnerMax * (tanh(aP1 * mP1Scale) * (1.0 / 2.0) + 1.0 / 2.0) + (mInnerMax - mInnerMax * (tanh(aP1 * mP1Scale) * (1.0 / 2.0) + 1.0 / 2.0)) * (tanh(aP2 * mP2Scale) * (1.0 / 2.0) + 1.0 / 2.0);
		else aOuter = aInner + mEdgeMin; 
	}
}


void ConeAngleTanhParameterization::derivativeChain(double& aDfDp1, double& aDfDp2, const double aDfDi, const double aDfDo, const double aP1, const double aP2) const
{
	aDfDp1 = aDfDp2 = 0.0;
	if (mP1Active) {
		const double didp1 = mInnerMax * mP1Scale * (pow(tanh(aP1 * mP1Scale), 2.0) - 1.0) * (-1.0 / 2.0);
		double dodp1;
		if (mP2Active) {
			dodp1 = mInnerMax * mP1Scale * (tanh(aP2 * mP2Scale) - 1.0) * (pow(tanh(aP1 * mP1Scale), 2.0) - 1.0) * (1.0 / 4.0);
			const double dodp2 = mInnerMax * mP2Scale * (tanh(aP1 * mP1Scale) - 1.0) * (pow(tanh(aP2 * mP2Scale), 2.0) - 1.0) * (1.0 / 4.0);
			aDfDp2 = aDfDo * dodp2;
		}
		else dodp1 = didp1; 
		aDfDp1 = aDfDi * didp1 + aDfDo * dodp1;
	}
}


void ConeAngleTanhParameterization::valuesToParams(double& aP1, double& aP2, const double aInner, const double aOuter) const
{
	if (mP1Active) {
		if (aInner < 1e-5) aP1 = -5.0 / mP1Scale;
		else if (aInner >(mInnerMax - 1e-5)) aP1 = 5.0 / mP1Scale; 
		else {
			aP1 = atanh((aInner * 2.0) / mInnerMax - 1.0) / mP1Scale;
		}
		if (mP2Active) {
			if (aOuter < (aInner + mEdgeMin + 1e-5)) aP2 = -5.0 / mP2Scale;
			else if (aOuter > 1.57079) aP2 = 5.0 / mP2Scale;
			else {
				aP2 = atanh(-(mInnerMax + aOuter * 4.0 - 3.141592653589793 * 2.0 - mInnerMax * tanh(aP1 * mP1Scale)) / (mInnerMax * (tanh(aP1 * mP1Scale) - 1.0))) / mP2Scale;
			}
		}
	}
}
