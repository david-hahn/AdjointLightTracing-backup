#pragma once

#include <tamashii/core/forward.h>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/matrix.hpp>
#include <tamashii/core/scene/light.hpp>

T_BEGIN_PYTHON_NAMESPACE

class Light {
public:
	Light() : lightRef{ nullptr } {}
	explicit Light(std::shared_ptr<RefLight> l) : lightRef{ std::move(l) } {}
	virtual ~Light()= default;

	static std::unique_ptr<Light> create(std::shared_ptr<RefLight>);
	
	[[nodiscard]] std::string_view getName() const;
	void setName(const std::string&) const;

	tamashii::Light::Type getType() const;
	PointLight asPointLight() const;
	DirectionalLight asDirectionalLight() const;

	[[nodiscard]] std::array<float, 3> getColor() const;
	void setColor(std::array<float, 3>) const;
	[[nodiscard]] float getIntensity() const;
	void setIntensity(float) const;

	void setModelMatrix(const Matrix4x4<float>&) const;
	[[nodiscard]] Matrix4x4<float> getModelMatrix() const;
	[[nodiscard]] std::array<float, 3> getPosition() const;
	void setPosition(std::array<float, 3>) const;

	void attachToScene(tamashii::RenderScene&);

	[[nodiscard]] bool isAttached() const;
	[[nodiscard]] const RefLight& refLight() const;

protected:
	void checkValidity() const;
	void updateModelMatrix() const;
	virtual std::unique_ptr<tamashii::Light> createLight() { return {}; }

	std::shared_ptr<RefLight> lightRef;
};

class PythonLightTrampoline final : public Light {
	NB_TRAMPOLINE(Light, 0);
};

class PointLight final : public Light {
public:
	using Light::Light;

	[[nodiscard]] float getRange() const;
	[[nodiscard]] float getRadius() const;
	void setRange(float) const;
	void setRadius(float) const;

private:
	tamashii::PointLight& pointLight() const;
	std::unique_ptr<tamashii::Light> createLight() override;
};

class DirectionalLight final : public Light {
public:
	using Light::Light;

	[[nodiscard]] float getAngle() const;
	void setAngle(float) const;

private:
	tamashii::DirectionalLight& directionalLight() const;
	std::unique_ptr<tamashii::Light> createLight() override;
};

T_END_PYTHON_NAMESPACE