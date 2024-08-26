#pragma once

#include "../renderer.hpp"

T_BEGIN_PYTHON_NAMESPACE

class OptimizationParameters {
public:
	OptimizationParameters(InteractiveAdjointLightTracing& i, const Ref& r)
		: ialt{ i }, ref{ r } {}

	void reset() const;
	void setDefault() const;
	void setAll() const;

	bool get(LightOptParams::PARAMS) const;
	void set(LightOptParams::PARAMS, bool) const;

	std::string toReprString() const;

protected:
	LightOptParams& params() const;

	InteractiveAdjointLightTracing& ialt;
	const Ref& ref;
};

class OptimizationParametersLight : public OptimizationParameters {
public:
	OptimizationParametersLight(InteractiveAdjointLightTracing& i, const Ref& r) : OptimizationParameters{i, r} {}

	void setRotations() const;
	void setPositions() const;
};

class OptimizationParametersMesh : public OptimizationParameters {
public:
	OptimizationParametersMesh(InteractiveAdjointLightTracing& i, const Ref& r) : OptimizationParameters{ i, r } {}

	bool getEmissiveStrength() const;
	bool getEmissiveFactor() const;
	void setEmissiveStrength(bool) const;
	void setEmissiveFactor(bool) const;
};

T_END_PYTHON_NAMESPACE