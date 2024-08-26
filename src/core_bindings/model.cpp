#include <tamashii/bindings/model.hpp>
#include <tamashii/core/common/common.hpp>

#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/material.hpp>
#include <tamashii/core/scene/ref_entities.hpp>

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS

std::string_view python::Mesh::getName() const
{
    checkValidity();
    return meshRef->mesh->getName();
}

void python::Mesh::setName(const std::string& name) const
{
    checkValidity();
    meshRef->mesh->setName(name);
}

float python::Mesh::getEmissionStrength() const
{
    checkValidity();
    return meshRef->mesh->getMaterial()->getEmissionStrength();
}

void python::Mesh::setEmissionStrength(const float strength) const
{
    checkValidity();
    meshRef->mesh->getMaterial()->setEmissionStrength(strength);
    Common::getInstance().getRenderSystem()->getMainScene()->requestMaterialUpdate();
    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

std::array<float, 3> python::Mesh::getEmissionFactor() const
{
    checkValidity();
    const auto c = meshRef->mesh->getMaterial()->getEmissionFactor();
    return { c.x, c.y, c.z };
}

void python::Mesh::setEmissionFactor(std::array<float, 3> factor) const
{
    checkValidity();
    meshRef->mesh->getMaterial()->setEmissionFactor({ factor[0], factor[1], factor[2] });
    Common::getInstance().getRenderSystem()->getMainScene()->requestMaterialUpdate();
    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

bool python::Mesh::isAttached() const
{
    if (!meshRef) {
        return false;
    }

    checkValidity();
    const auto& modelList = Common::getInstance().getRenderSystem()->getMainScene()->getModelList();
    for (const auto& model : modelList)
    {
        for (const auto& mesh : model->refMeshes)
        {
            if (mesh == meshRef) return true;
        }
    }
    return false;
}

const RefMesh& python::Mesh::refMesh() const
{
    checkValidity();
    return *meshRef;
}

void python::Mesh::checkValidity() const
{
    if (!meshRef) {
        throw std::runtime_error("Light needs to be attached before use");
    }

    const auto* renderSystem = Common::getInstance().getRenderSystem();
    if (!renderSystem) {
        throw std::runtime_error("No active render system.");
    }

    auto& mainScene = renderSystem->getMainScene();
    if (!mainScene) {
        throw std::runtime_error("No active main scene.");
    }
}

std::string_view python::Model::getName() const
{
    checkValidity();
    return modelRef->model->getName();
}

void python::Model::setName(const std::string& name) const
{
    checkValidity();
    modelRef->model->setName(name);
}

std::vector<std::unique_ptr<python::Mesh>> python::Model::getMeshes() const
{
    const auto& meshes = modelRef->refMeshes;
    std::vector<std::unique_ptr<python::Mesh>> pythonMeshes;
    pythonMeshes.reserve(meshes.size());
    for (auto& mesh : meshes) {
        pythonMeshes.emplace_back(std::make_unique<Mesh>(mesh));
    }
    return pythonMeshes;
}

bool python::Model::isAttached() const
{
    if (!modelRef) {
        return false;
    }

    checkValidity();
    const auto& modelList = Common::getInstance().getRenderSystem()->getMainScene()->getModelList();
    for (const auto& m : modelList)
    {
        if (m == modelRef) return true;
    }
    return false;
}

const RefModel& python::Model::refModel() const
{
    checkValidity();
    return *modelRef;
}

void python::Model::checkValidity() const {
    if (!modelRef) {
        throw std::runtime_error("Models needs to be attached before use");
    }

    const auto* renderSystem = Common::getInstance().getRenderSystem();
    if (!renderSystem) {
        throw std::runtime_error("No active render system.");
    }

    auto& mainScene = renderSystem->getMainScene();
    if (!mainScene) {
        throw std::runtime_error("No active main scene.");
    }
}
