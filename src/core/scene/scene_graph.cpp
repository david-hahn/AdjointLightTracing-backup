#include <tamashii/core/scene/scene_graph.hpp>
#include <tamashii/core/common/math.hpp>

T_USE_NAMESPACE

TRS::TRS() : translation{0.0f}, translationInterpolation{Interpolation::NONE}, rotation{0.0f},
             rotationInterpolation{Interpolation::NONE}, scale{1.0f},
             scaleInterpolation{Interpolation::NONE}
{
}

bool TRS::hasTranslation() const
{
	return any(notEqual(translation, glm::vec3{0.0f}));
}

bool TRS::hasRotation() const
{
	return any(notEqual(rotation, glm::vec4{0.0f}));
}

bool TRS::hasScale() const
{
	return any(notEqual(scale, glm::vec3{1.0f}));
}

bool TRS::hasTranslationAnimation() const
{
	return (translationInterpolation != Interpolation::NONE) && !translationTimeSteps.empty();
}

bool TRS::hasRotationAnimation() const
{
	return (rotationInterpolation != Interpolation::NONE) && !rotationTimeSteps.empty();
}

bool TRS::hasScaleAnimation() const
{
	return (scaleInterpolation != Interpolation::NONE) && !scaleTimeSteps.empty();
}

glm::mat4 TRS::getMatrix(const float aTime) const
{
	const float time = std::isnan(aTime) ? 0.0f : aTime;
	auto localModelMatrix = glm::mat4{1.0f};
	computeTranslation(time, localModelMatrix);
	computeRotation(time, localModelMatrix);
	computeScale(time, localModelMatrix);
	return localModelMatrix;
}

void TRS::computeTranslation(const float& aTime, glm::mat4& aLocalModelMatrix) const
{
	if (translationInterpolation == Interpolation::LINEAR)
	{
		size_t previous = 0;
		size_t next = translationTimeSteps.size() - 1;
		for (size_t i = 0; i < translationTimeSteps.size(); i++)
		{
			const float timeStep = translationTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime)
			{
				next = i;
				break;
			}
		}
		const float interpolationValue = (aTime - translationTimeSteps.at(previous)) / (translationTimeSteps.at(next) -
			translationTimeSteps.at(previous));
		glm::vec3 currentTranslation;
		
		if (std::isinf(interpolationValue)) currentTranslation = translationSteps.at(previous);
		else currentTranslation = mix(translationSteps.at(previous), translationSteps.at(next), interpolationValue);
		aLocalModelMatrix = translate(aLocalModelMatrix, currentTranslation);
	}
	else if (translationInterpolation == Interpolation::STEP)
	{
		size_t previous = 0;
		for (size_t i = 0; i < translationTimeSteps.size(); i++)
		{
			const float timeStep = translationTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime) break;
		}
		aLocalModelMatrix = translate(aLocalModelMatrix, translationSteps.at(previous));
	}
	else if (translationInterpolation == Interpolation::CUBIC_SPLINE)
	{
		size_t previous = 0;
		size_t next = translationTimeSteps.size() - 1;
		for (size_t i = 0; i < translationTimeSteps.size(); i++)
		{
			const float timeStep = translationTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime)
			{
				next = i;
				break;
			}
		}
		const float deltaTime = translationTimeSteps.at(next) - translationTimeSteps.at(previous);
		const glm::vec3 previousTangent = deltaTime * translationSteps.at((previous * 3) + 2);
		const glm::vec3 nextTangent = deltaTime * translationSteps.at((next * 3));
		const glm::vec3 previousValue = translationSteps.at((previous * 3) + 1);
		const glm::vec3 nextValue = translationSteps.at((next * 3) + 1);

		if (deltaTime == 0.0f)
		{
			aLocalModelMatrix = translate(aLocalModelMatrix, previousValue);
			return;
		}

		const float t = (aTime - translationTimeSteps.at(previous)) / (translationTimeSteps.at(next) -
			translationTimeSteps.at(previous));
		const float t2 = t * t;
		const float t3 = t2 * t;

		const glm::vec3 currentTranslation = (2 * t3 - 3 * t2 + 1) * previousValue + (t3 - 2 * t2 + t) * previousTangent
			+ (-2
				* t3 + 3 * t2) * nextValue + (t3 - t2) * nextTangent;
		aLocalModelMatrix = translate(aLocalModelMatrix, currentTranslation);
	}
	else if (hasTranslation()) aLocalModelMatrix = translate(aLocalModelMatrix, translation);
}

void TRS::computeRotation(const float& aTime, glm::mat4& aLocalModelMatrix) const
{
	if (rotationInterpolation == Interpolation::LINEAR)
	{
		size_t previous = 0;
		size_t next = rotationTimeSteps.size() - 1;
		for (size_t i = 0; i < rotationTimeSteps.size(); i++)
		{
			const float timeStep = rotationTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime)
			{
				next = i;
				break;
			}
		}
		const float interpolationValue = (aTime - rotationTimeSteps.at(previous)) / (rotationTimeSteps.at(next) -
			rotationTimeSteps.at(previous));
		glm::vec4 prev_rot = rotationSteps.at(previous);
		glm::vec4 next_rot = rotationSteps.at(next);
		
		if (std::isinf(interpolationValue))
		{
			aLocalModelMatrix *= toMat4(glm::quat{prev_rot[3], prev_rot[0], prev_rot[1], prev_rot[2]});
			return;
		}
		const glm::quat interpolatedQuat = slerp(glm::quat{prev_rot[3], prev_rot[0], prev_rot[1], prev_rot[2]},
		                                         glm::quat{next_rot[3], next_rot[0], next_rot[1], next_rot[2]},
		                                         interpolationValue);
		aLocalModelMatrix *= toMat4(interpolatedQuat);
	}
	else if (rotationInterpolation == Interpolation::STEP)
	{
		size_t previous = 0;
		for (size_t i = 0; i < rotationTimeSteps.size(); i++)
		{
			const float timeStep = rotationTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime) break;
		}
		glm::vec4 prevRot = rotationSteps.at(previous);
		const auto prevQuat = glm::quat{prevRot[3], prevRot};
		aLocalModelMatrix *= toMat4(prevQuat);
	}
	else if (rotationInterpolation == Interpolation::CUBIC_SPLINE)
	{
		size_t previous = 0;
		size_t next = rotationTimeSteps.size() - 1;
		for (size_t i = 0; i < rotationTimeSteps.size(); i++)
		{
			const float timeStep = rotationTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime)
			{
				next = i;
				break;
			}
		}
		const float deltaTime = rotationTimeSteps.at(next) - rotationTimeSteps.at(previous);
		const glm::vec4 previousTangent = deltaTime * rotationSteps.at((previous * 3) + 2);
		const glm::vec4 nextTangent = deltaTime * rotationSteps.at((next * 3));
		glm::vec4 previousValue = rotationSteps.at((previous * 3) + 1);
		const glm::vec4 nextValue = rotationSteps.at((next * 3) + 1);

		if (deltaTime == 0.0f)
		{
			aLocalModelMatrix *= toMat4(
				normalize(glm::quat{previousValue[3], previousValue[0], previousValue[1], previousValue[2]}));
			return;
		}

		const float t = (aTime - rotationTimeSteps.at(previous)) / (rotationTimeSteps.at(next) - rotationTimeSteps.
			at(previous));
		const float t2 = t * t;
		const float t3 = t2 * t;

		glm::vec4 currentRotation = (2 * t3 - 3 * t2 + 1) * previousValue + (t3 - 2 * t2 + t) * previousTangent + (-2 *
			t3 + 3 * t2) * nextValue + (t3 - t2) * nextTangent;
		aLocalModelMatrix *= toMat4(normalize(glm::quat{
			currentRotation[3], currentRotation[0], currentRotation[1], currentRotation[2]
		}));
	}
	else if (hasRotation())
		aLocalModelMatrix *= toMat4(
			glm::quat{rotation[3], rotation[0], rotation[1], rotation[2]});
}

void TRS::computeScale(const float& aTime, glm::mat4& aLocalModelMatrix) const
{
	if (scaleInterpolation == Interpolation::LINEAR)
	{
		size_t previous = 0;
		size_t next = scaleTimeSteps.size() - 1;
		for (size_t i = 0; i < scaleTimeSteps.size(); i++)
		{
			const float timeStep = scaleTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime)
			{
				next = i;
				break;
			}
		}
		const float interpolationValue = (aTime - scaleTimeSteps.at(previous)) / (scaleTimeSteps.at(next) -
			scaleTimeSteps.at(previous));
		glm::vec3 currentScale;
		
		if (std::isinf(interpolationValue)) currentScale = scaleSteps.at(previous);
		else currentScale = mix(scaleSteps.at(previous), scaleSteps.at(next), interpolationValue);
		aLocalModelMatrix = glm::scale(aLocalModelMatrix, currentScale);
	}
	else if (scaleInterpolation == Interpolation::STEP)
	{
		size_t previous = 0;
		for (size_t i = 0; i < scaleTimeSteps.size(); i++)
		{
			const float timeStep = scaleTimeSteps.at(i);
			if (timeStep <= aTime) previous = i;
			else if (timeStep >= aTime) break;
		}
		aLocalModelMatrix = glm::scale(aLocalModelMatrix, scaleSteps.at(previous));
	}
	else if (scaleInterpolation == Interpolation::CUBIC_SPLINE)
	{
		size_t previous = 0;
		size_t next = scaleTimeSteps.size() - 1;
		for (size_t i = 0; i < scaleTimeSteps.size(); i++)
		{
			const float time_step = scaleTimeSteps.at(i);
			if (time_step <= aTime) previous = i;
			else if (time_step >= aTime)
			{
				next = i;
				break;
			}
		}
		const float deltaTime = scaleTimeSteps.at(next) - scaleTimeSteps.at(previous);
		const glm::vec3 previousTangent = deltaTime * scaleSteps.at((previous * 3) + 2);
		const glm::vec3 nextTangent = deltaTime * scaleSteps.at((next * 3));
		const glm::vec3 previousValue = scaleSteps.at((previous * 3) + 1);
		const glm::vec3 nextValue = scaleSteps.at((next * 3) + 1);

		if (deltaTime == 0.0f)
		{
			aLocalModelMatrix = glm::scale(aLocalModelMatrix, previousValue);
			return;
		}

		const float t = (aTime - scaleTimeSteps.at(previous)) / (scaleTimeSteps.at(next) - scaleTimeSteps.at(previous));
		const float t2 = t * t;
		const float t3 = t2 * t;

		const glm::vec3 currentScale = (2 * t3 - 3 * t2 + 1) * previousValue + (t3 - 2 * t2 + t) * previousTangent + (-2
			* t3 + 3 * t2) * nextValue + (t3 - t2) * nextTangent;
		aLocalModelMatrix = glm::scale(aLocalModelMatrix, currentScale);
	}
	else if (hasScale()) aLocalModelMatrix = glm::scale(aLocalModelMatrix, scale);
}

Node::Node(const std::string_view aName) : Asset{Type::NODE, aName}, mModel{nullptr},
                                           mCamera{nullptr}, mLight{nullptr}
{
}

std::unique_ptr<Node> Node::alloc(const std::string_view aName)
{
	return std::make_unique<Node>(aName);
}

Node& Node::addChildNode(std::string_view aName)
{
	mChildren.emplace_back(std::make_unique<Node>(aName));
	return *mChildren.back();
}

Node& Node::addChildNode(std::unique_ptr<Node>& aNode)
{
	mChildren.emplace_back(std::move(aNode));
	return *mChildren.back();
}

size_t Node::numChildNodes() const
{
	return mChildren.size();
}

Node& Node::getChildNode(const size_t aIndex) const
{
	return *mChildren[aIndex];
}

std::deque<std::unique_ptr<Node>>& Node::getChildNodes()
{
	return mChildren;
}

bool Node::hasModel() const
{
	return mModel != nullptr;
}

bool Node::hasCamera() const
{
	return mCamera != nullptr;
}

bool Node::hasLight() const
{
	return mLight != nullptr;
}

const std::shared_ptr<Model>& Node::getModel() const
{
	return mModel;
}

const std::shared_ptr<Camera>& Node::getCamera() const
{
	return mCamera;
}

const std::shared_ptr<Light>& Node::getLight() const
{
	return mLight;
}

void Node::setModel(const std::shared_ptr<Model>& aModel)
{
	mModel = aModel;
}

void Node::setCamera(const std::shared_ptr<Camera>& aCamera)
{
	mCamera = aCamera;
}

void Node::setLight(const std::shared_ptr<Light>& aLight)
{
	mLight = aLight;
}

TRS& Node::getTRS()
{
	return mTrs;
}

bool Node::hasAnimation() const
{
	return hasTranslationAnimation() || hasRotationAnimation() || hasScaleAnimation();
}

bool Node::hasLocalTransform() const
{
	return hasTranslation() || hasRotation() || hasScale() || hasAnimation();
}

bool Node::hasTranslation() const
{
	return mTrs.hasTranslation();
}

bool Node::hasRotation() const
{
	return mTrs.hasRotation();
}

bool Node::hasScale() const
{
	return mTrs.hasScale();
}

bool Node::hasTranslationAnimation() const
{
	return mTrs.hasTranslationAnimation();
}

bool Node::hasRotationAnimation() const
{
	return mTrs.hasRotationAnimation();
}

bool Node::hasScaleAnimation() const
{
	return mTrs.hasScaleAnimation();
}

void Node::setTranslationAnimation(const TRS::Interpolation aInterpolation, const std::vector<float>& aTimeSteps,
                                   const std::vector<glm::vec3>& aTranslationSteps)
{
	mTrs.translationInterpolation = aInterpolation;
	mTrs.translationTimeSteps = aTimeSteps;
	mTrs.translationSteps = aTranslationSteps;
}

void Node::setRotationAnimation(const TRS::Interpolation aInterpolation, const std::vector<float>& aTimeSteps,
                                const std::vector<glm::vec4>& aRotationSteps)
{
	mTrs.rotationInterpolation = aInterpolation;
	mTrs.rotationTimeSteps = aTimeSteps;
	mTrs.rotationSteps = aRotationSteps;
}

void Node::setScaleAnimation(const TRS::Interpolation aInterpolation, const std::vector<float>& aTimeSteps,
                             const std::vector<glm::vec3>& aScaleSteps)
{
	mTrs.scaleInterpolation = aInterpolation;
	mTrs.scaleTimeSteps = aTimeSteps;
	mTrs.scaleSteps = aScaleSteps;
}

std::deque<std::unique_ptr<Node>>::iterator Node::begin()
{
	return mChildren.begin();
}

std::deque<std::unique_ptr<Node>>::const_iterator Node::begin() const
{
	return mChildren.begin();
}

std::deque<std::unique_ptr<Node>>::iterator Node::end()
{
	return mChildren.end();
}

std::deque<std::unique_ptr<Node>>::const_iterator Node::end() const
{
	return mChildren.end();
}

glm::mat4 Node::getLocalModelMatrix() const
{
	auto localModelMatrix = glm::mat4{1.0f};
	if (hasTranslation()) localModelMatrix = translate(localModelMatrix, mTrs.translation);
	if (hasRotation()) localModelMatrix *= toMat4(glm::quat{
		mTrs.rotation[3], mTrs.rotation[0], mTrs.rotation[1], mTrs.rotation[2]
	});
	if (hasScale()) localModelMatrix = scale(localModelMatrix, mTrs.scale);
	return localModelMatrix;
}

void Node::setTranslation(const glm::vec3& aTranslation)
{
	mTrs.translation = aTranslation;
}

void Node::setRotation(const glm::vec4& aRotation)
{
	mTrs.rotation = aRotation;
}

void Node::setScale(const glm::vec3& aScale)
{
	mTrs.scale = aScale;
}

void Node::setModelMatrix(const glm::mat4& aModelMatrix)
{
	glm::quat rotation{};
	const bool error = math::decomposeTransform(aModelMatrix, mTrs.translation, rotation, mTrs.scale);
	ASSERT(error, "decompose error");
	mTrs.rotation = glm::vec4{rotation.x, rotation.y, rotation.z, rotation.w};
}

void Node::visit(const std::function<void(Node*)>& aFunction)
{
	aFunction(this);
	for (const auto& n : mChildren) n->visit(aFunction);
}
