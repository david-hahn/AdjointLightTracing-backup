#include <tamashii/bindings/light.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/core/scene/ref_entities.hpp>

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS

std::unique_ptr<python::Light> python::Light::create(std::shared_ptr<RefLight> light) {
    switch (light->light->getType()) {
    case tamashii::Light::Type::DIRECTIONAL:
        return std::make_unique<DirectionalLight>(std::move(light));
    case tamashii::Light::Type::POINT:
        return std::make_unique<PointLight>(std::move(light));
    default:
        
        return std::make_unique<Light>(light);
    }
}

void python::Light::checkValidity() const {
    if (!lightRef) {
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

void python::Light::updateModelMatrix() const
{
    lightRef->position = lightRef->model_matrix * glm::vec4(0, 0, 0, 1);
    lightRef->direction = glm::normalize(lightRef->model_matrix * lightRef->light->getDefaultDirection());

    
    if (lightRef->light->getType() == tamashii::Light::Type::SURFACE) {
        const float scale_x = glm::length(glm::vec3(lightRef->model_matrix[0]));
        const float scale_y = glm::length(glm::vec3(lightRef->model_matrix[1]));
        const float scale_z = glm::length(glm::vec3(lightRef->model_matrix[2]));
        dynamic_cast<SurfaceLight&>(*lightRef->light).setDimensions(glm::vec3(scale_x, scale_y, scale_z));
    }

    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

bool python::Light::isAttached() const
{
    if (!lightRef) {
        return false;
    }

    checkValidity();
    const auto& lightList = Common::getInstance().getRenderSystem()->getMainScene()->getLightList();
    for (const auto& l : lightList)
    {
        if (l == lightRef) return true;
    }
    return false;
}

const RefLight& python::Light::refLight() const {
    checkValidity();
    return *lightRef;
}

std::string_view python::Light::getName() const
{
    checkValidity();
    return lightRef->light->getName();
}

void python::Light::setName(const std::string& name) const
{
    checkValidity();
    lightRef->light->setName(name);
}

tamashii::Light::Type python::Light::getType() const
{
    checkValidity();
    return lightRef->light->getType();
}

python::PointLight python::Light::asPointLight() const
{
    checkValidity();
    if (getType() != tamashii::Light::Type::POINT) {
        throw std::runtime_error("PythonLight is not a point light, cannot downcast.");
    }
    return PointLight{ lightRef };
}

python::DirectionalLight python::Light::asDirectionalLight() const
{
    checkValidity();
    if (getType() != tamashii::Light::Type::DIRECTIONAL) {
        throw std::runtime_error("PythonLight is not a directional light, cannot downcast.");
    }
    return DirectionalLight{ lightRef };
}

std::array<float, 3> python::Light::getColor() const
{
    checkValidity();
    const auto c = lightRef->light->getColor();
    return { c.x, c.y, c.z };
}

void python::Light::setColor(std::array<float, 3> c) const
{
    checkValidity();
    lightRef->light->setColor({ c[0],c[1],c[2] });
    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

float python::Light::getIntensity() const
{
    return lightRef->light->getIntensity();
}

void python::Light::setIntensity(const float i) const
{
    checkValidity();
    lightRef->light->setIntensity(i);
    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

std::array<float, 3> python::Light::getPosition() const
{
    checkValidity();
    return { lightRef->model_matrix[3][0], lightRef->model_matrix[3][1], lightRef->model_matrix[3][2] };
}

void python::Light::setPosition(const std::array<float, 3> p) const
{
    checkValidity();
    lightRef->model_matrix[3][0] = p[0];
    lightRef->model_matrix[3][1] = p[1];
    lightRef->model_matrix[3][2] = p[2];
    updateModelMatrix();
}

void python::Light::setModelMatrix(const Matrix4x4<float>& pyMatrix) const
{
    checkValidity();
    const auto modelMat = toGlmMat4x4(pyMatrix);
    lightRef->model_matrix = modelMat;
    updateModelMatrix();
}

Matrix4x4<float> python::Light::getModelMatrix() const
{
    checkValidity();

    
    const auto matrixHandle = new glm::mat4{ lightRef->model_matrix };
    const nb::capsule owner(matrixHandle, "matrix capsule", [](void* ptr) noexcept {
        delete static_cast<glm::mat4*>(ptr);
        });
    constexpr size_t shape[2] = { 4, 4 };
    return { glm::value_ptr(*matrixHandle), 2, shape, owner };
}

void python::Light::attachToScene(tamashii::RenderScene& scene)
{
    if (isAttached()) {
        throw std::runtime_error{ "Light already attached" };
    }

    auto quat = glm::quat{ glm::radians(glm::vec3{ -90, 0, 0 }) };
    auto light = createLight();
    lightRef = scene.addLightRef(std::move(light), glm::vec3{ 0 }, { quat[1], quat[2], quat[3] , quat[0] });
}

float python::PointLight::getRange() const
{
    checkValidity();
    return pointLight().getRange();
}

float python::PointLight::getRadius() const
{
    checkValidity();
    return pointLight().getRadius();
}

void python::PointLight::setRange(const float range) const
{
    checkValidity();
    pointLight().setRange(range);
    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

void python::PointLight::setRadius(const float radius) const
{
    checkValidity();
    pointLight().setRadius(radius);
    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

tamashii::PointLight& python::PointLight::pointLight() const
{
    return dynamic_cast<tamashii::PointLight&>(*lightRef->light);
}

std::unique_ptr<tamashii::Light> python::PointLight::createLight()
{
    return std::make_unique<tamashii::PointLight>();
}

float python::DirectionalLight::getAngle() const
{
    checkValidity();
    return directionalLight().getAngle();
}

void python::DirectionalLight::setAngle(const float angle) const
{
    checkValidity();
    directionalLight().setAngle(angle);
    Common::getInstance().getRenderSystem()->getMainScene()->requestLightUpdate();
}

tamashii::DirectionalLight& python::DirectionalLight::directionalLight() const
{
    return dynamic_cast<tamashii::DirectionalLight&>(*lightRef->light);
}

std::unique_ptr<tamashii::Light> python::DirectionalLight::createLight()
{
    return std::make_unique<tamashii::DirectionalLight>();
}