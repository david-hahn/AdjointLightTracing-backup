#pragma once

#include <tamashii/core/forward.h>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/matrix.hpp>

T_BEGIN_PYTHON_NAMESPACE

class Camera {
public:
	Camera() : cameraRef{ nullptr } {}
	explicit Camera(std::shared_ptr<RefCamera> c) : cameraRef{ std::move(c) } {}
	virtual ~Camera() = default;

	std::string_view getName() const;
	void setName(const std::string&) const;

	void setModelMatrix(const Matrix4x4<float>&) const;
	Matrix4x4<float> getModelMatrix() const;
	void lookAt(const Vector3<float>& pyEye, const Vector3<float>& pyCenter) const;

	std::array<float, 3> getPosition() const;
	void setPosition(std::array<float, 3>) const;

	void makeCurrentCamera() const;

	bool isAttached() const;

protected:
	void checkValidity() const;
	void updateModelMatrix(glm::mat4&) const;

	std::shared_ptr<RefCamera> cameraRef;
};

T_END_PYTHON_NAMESPACE