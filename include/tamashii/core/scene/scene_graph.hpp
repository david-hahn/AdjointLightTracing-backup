#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>
#include <tamashii/core/scene/asset.hpp>

#include <deque>
#include <vector>
#include <functional>

T_BEGIN_NAMESPACE

struct TRS
{
	enum class Interpolation
	{
		NONE,
		LINEAR,
		STEP,
		CUBIC_SPLINE
	};

	TRS();

	glm::vec3 translation;
	Interpolation translationInterpolation;
	std::vector<float> translationTimeSteps;
	std::vector<glm::vec3> translationSteps;

	glm::vec4 rotation; 
	Interpolation rotationInterpolation;
	std::vector<float> rotationTimeSteps;
	std::vector<glm::vec4> rotationSteps;

	glm::vec3 scale;
	Interpolation scaleInterpolation;
	std::vector<float> scaleTimeSteps;
	std::vector<glm::vec3> scaleSteps;

	[[nodiscard]] bool hasTranslation() const;
	[[nodiscard]] bool hasRotation() const;
	[[nodiscard]] bool hasScale() const;
	[[nodiscard]] bool hasTranslationAnimation() const;
	[[nodiscard]] bool hasRotationAnimation() const;
	[[nodiscard]] bool hasScaleAnimation() const;

	[[nodiscard]] glm::mat4 getMatrix(float aTime /* in seconds */) const;

private:
	void computeTranslation(const float& aTime, glm::mat4& aLocalModelMatrix) const;
	void computeRotation(const float& aTime, glm::mat4& aLocalModelMatrix) const;
	void computeScale(const float& aTime, glm::mat4& aLocalModelMatrix) const;
};

class Node final : public Asset
{
public:
	explicit Node(std::string_view aName);
	~Node() override = default;
	T_DELETE_MOVE_COPY_CONSTRUCTOR(Node)

	static std::unique_ptr<Node> alloc(std::string_view aName);

	
	Node& addChildNode(std::string_view aName);
	Node& addChildNode(std::unique_ptr<Node>& aNode);
	[[nodiscard]] size_t numChildNodes() const;
	[[nodiscard]] Node& getChildNode(size_t aIndex) const;
	std::deque<std::unique_ptr<Node>>& getChildNodes();

	
	[[nodiscard]] bool hasModel() const;
	[[nodiscard]] bool hasCamera() const;
	[[nodiscard]] bool hasLight() const;
	[[nodiscard]] const std::shared_ptr<Model>& getModel() const;
	[[nodiscard]] const std::shared_ptr<Camera>& getCamera() const;
	[[nodiscard]] const std::shared_ptr<Light>& getLight() const;


	void setModel(const std::shared_ptr<Model>& aModel);
	void setCamera(const std::shared_ptr<Camera>& aCamera);
	void setLight(const std::shared_ptr<Light>& aLight);


	
	TRS& getTRS();
	[[nodiscard]] glm::mat4 getLocalModelMatrix() const;
	[[nodiscard]] bool hasAnimation() const;
	[[nodiscard]] bool hasLocalTransform() const;
	[[nodiscard]] bool hasTranslation() const;
	[[nodiscard]] bool hasRotation() const;
	[[nodiscard]] bool hasScale() const;
	[[nodiscard]] bool hasTranslationAnimation() const;
	[[nodiscard]] bool hasRotationAnimation() const;
	[[nodiscard]] bool hasScaleAnimation() const;

	void setTranslation(const glm::vec3& aTranslation);
	void setRotation(const glm::vec4& aRotation);
	void setScale(const glm::vec3& aScale);
	void setModelMatrix(const glm::mat4& aModelMatrix);

	void setTranslationAnimation(TRS::Interpolation aInterpolation, const std::vector<float>& aTimeSteps,
	                             const std::vector<glm::vec3>& aTranslationSteps);
	void setRotationAnimation(TRS::Interpolation aInterpolation, const std::vector<float>& aTimeSteps,
	                          const std::vector<glm::vec4>& aRotationSteps);
	void setScaleAnimation(TRS::Interpolation aInterpolation, const std::vector<float>& aTimeSteps,
	                       const std::vector<glm::vec3>& aScaleSteps);
	void visit(const std::function<void(Node*)>& aFunction);

	
	std::deque<std::unique_ptr<Node>>::iterator begin();
	[[nodiscard]] std::deque<std::unique_ptr<Node>>::const_iterator begin() const;
	std::deque<std::unique_ptr<Node>>::iterator end();
	[[nodiscard]] std::deque<std::unique_ptr<Node>>::const_iterator end() const;

private:
	std::deque<std::unique_ptr<Node>> mChildren;

	
	std::shared_ptr<Model> mModel;
	std::shared_ptr<Camera> mCamera;
	std::shared_ptr<Light> mLight;

	
	TRS mTrs;
};

T_END_NAMESPACE
