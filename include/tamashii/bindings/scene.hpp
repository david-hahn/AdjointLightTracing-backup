#pragma once

#include <tamashii/bindings/bindings.hpp>

T_BEGIN_PYTHON_NAMESPACE
class Scene {
public:

	Scene(std::weak_ptr<tamashii::RenderScene> weakScene);

	[[nodiscard]] std::vector<std::unique_ptr<Model>> getModels() const;

	[[nodiscard]] std::vector<std::unique_ptr<Light>> getLights() const;
	void addLight(Light&) const;

	[[nodiscard]] std::vector<std::unique_ptr<Camera>> getCameras() const;

	[[nodiscard]] Camera getCurrentCamera() const;

private:
	std::shared_ptr<tamashii::RenderScene> tryGetRenderScene() const;

	std::weak_ptr<tamashii::RenderScene> renderScene;
};

T_END_PYTHON_NAMESPACE
