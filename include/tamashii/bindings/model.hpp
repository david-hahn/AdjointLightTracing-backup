#pragma once

#include <tamashii/core/forward.h>
#include <tamashii/bindings/bindings.hpp>

T_BEGIN_PYTHON_NAMESPACE

class Mesh final
{
public:
	Mesh() : meshRef{ nullptr } {}
	explicit Mesh(std::shared_ptr<RefMesh> m) : meshRef{ std::move(m) } {}
	~Mesh() = default;

	[[nodiscard]] std::string_view getName() const;
	void setName(const std::string&) const;

	[[nodiscard]] float getEmissionStrength() const;
	void setEmissionStrength(float) const;

	[[nodiscard]] std::array<float, 3> getEmissionFactor() const;
	void setEmissionFactor(std::array<float, 3>) const;

	bool isAttached() const;
	const RefMesh& refMesh() const;

protected:
	void checkValidity() const;
	std::shared_ptr<RefMesh> meshRef;
};

class Model final
{
public:
	Model() : modelRef{ nullptr } {}
	explicit Model(std::shared_ptr<RefModel> m) : modelRef{ std::move(m) } {}
	~Model() = default;

	std::string_view getName() const;
	void setName(const std::string&) const;

	[[nodiscard]] std::vector<std::unique_ptr<Mesh>> getMeshes() const;

	bool isAttached() const;
	const RefModel& refModel() const;

protected:
	void checkValidity() const;
	std::shared_ptr<RefModel> modelRef;
};

T_END_PYTHON_NAMESPACE
