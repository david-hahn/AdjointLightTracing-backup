#pragma once

#include <tamashii/bindings/bindings.hpp>
#include <tamashii/bindings/model.hpp>
#include <tamashii/bindings/light.hpp>
#include <tamashii/bindings/scene.hpp>
#include <tamashii/bindings/camera.hpp>
#include <tamashii/bindings/ccli_variable.hpp>

#include <optional>

T_BEGIN_PYTHON_NAMESPACE

class Impls {};

class Exports final {
public:
	explicit Exports(nanobind::module_&);
    ~Exports() = default;

    
    Exports(const Exports&) = delete;
    Exports& operator=(const Exports&) = delete;
    Exports(Exports&&) = default;
    Exports& operator=(Exports&&) = default;

    auto& var() { return *mVar; }
    auto& impls() { return *mImpls; }
    auto& scene() { return *mScene; }
    auto& model() { return *mModel; }
    auto& mesh() { return *mMesh; }
    auto& light() { return *mLight; }
    auto& lightType() { return *mLightType; }
    auto& pointLight() { return *mPointLight; }
    auto& directionalLight() { return *mDirectionalLight; }
    auto& camera() { return *mCamera; }

private:
    
    std::optional<nb::class_<CcliVar>> mVar;
    std::optional<nb::class_<Impls>> mImpls;
    std::optional<nb::class_<Scene>> mScene;
    std::optional<nb::class_<Model>> mModel;
    std::optional<nb::class_<Mesh>> mMesh;
    std::optional<nb::class_<Light, PythonLightTrampoline>> mLight;
    std::optional<nb::enum_<tamashii::Light::Type>> mLightType;
    std::optional<nb::class_<PointLight, Light>> mPointLight;
    std::optional<nb::class_<DirectionalLight, Light>> mDirectionalLight;
    std::optional<nb::class_<Camera>> mCamera;
};

T_END_PYTHON_NAMESPACE