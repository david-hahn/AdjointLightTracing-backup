#include <tamashii/bindings/camera.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/bindings/bindings.hpp>
#include <tamashii/core/scene/ref_entities.hpp>
#include <tamashii/core/scene/camera.hpp>

T_USE_PYTHON_NAMESPACE
T_USE_NANOBIND_LITERALS

void python::Camera::checkValidity() const {
    if (!cameraRef) {
        throw std::runtime_error("Camera needs to be attached before use");
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

std::string_view python::Camera::getName() const {
    checkValidity();
    return cameraRef->camera->getName();
}

void python::Camera::setName(const std::string& name) const
{
    checkValidity();
    cameraRef->camera->setName(name);
}

void python::Camera::setModelMatrix(const Matrix4x4<float>& pyMatrix) const
{
    checkValidity();
    auto modelMat = toGlmMat4x4(pyMatrix);
    updateModelMatrix(modelMat);
}

void python::Camera::lookAt(const Vector3<float>& pyEye, const Vector3<float>& pyCenter) const {
    checkValidity();
    const auto eyeVec = toGlmVec3(pyEye);
    const auto centerVec = toGlmVec3(pyCenter);
    const auto viewMatrix = glm::lookAt(eyeVec, centerVec, glm::vec3{ 0, 1, 0 });
    auto modelMatrix = glm::inverse(viewMatrix);
    updateModelMatrix(modelMatrix);
}

std::array<float, 3> python::Camera::getPosition() const
{
    checkValidity();
    const auto p = cameraRef->getPosition();
    return { p[0], p[1] , p[2] };
}

void python::Camera::setPosition(std::array<float, 3> p) const
{
    checkValidity();
    auto& privateCam = reinterpret_cast<RefCameraPrivate&>(*cameraRef);
    privateCam.position = { p[0], p[1] , p[2] };
    privateCam.updateMatrix(true);
    Common::getInstance().getRenderSystem()->getMainScene()->requestCameraUpdate();
}

void python::Camera::makeCurrentCamera() const
{
    checkValidity();
    Common::getInstance().getRenderSystem()->getMainScene()->setCurrentCamera(cameraRef);
    Common::getInstance().getRenderSystem()->getMainScene()->requestCameraUpdate();
}

Matrix4x4<float> python::Camera::getModelMatrix() const {
    checkValidity();

    const auto& privateCam = reinterpret_cast<RefCameraPrivate&>(*cameraRef);
    const auto matrixHandle = new glm::mat4{ privateCam.model_matrix };

    if (privateCam.y_flipped) {
        (*matrixHandle)[0][1] *= -1;
        (*matrixHandle)[1][1] *= -1;
        (*matrixHandle)[2][1] *= -1;
        (*matrixHandle)[3][1] *= -1;
    }

    
    const nb::capsule owner(matrixHandle, "matrix capsule", [](void* ptr) noexcept {
        delete static_cast<glm::mat4*>(ptr);
        });
    constexpr size_t shape[2] = { 4, 4 };
    return { glm::value_ptr(*matrixHandle), 2, shape, owner };
}

void python::Camera::updateModelMatrix(glm::mat4& matrix) const
{
    dynamic_cast<RefCameraPrivate&>(*cameraRef).setModelMatrix(matrix, true);
    Common::getInstance().getRenderSystem()->getMainScene()->requestCameraUpdate();
}

bool python::Camera::isAttached() const
{
    if (!cameraRef) {
        return false;
    }

    checkValidity();
    const auto& currentCam = Common::getInstance().getRenderSystem()->getMainScene()->getCurrentCamera();
    return &currentCam == cameraRef.get();
}