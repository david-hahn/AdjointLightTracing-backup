#include <tamashii/core/scene/camera.hpp>

T_USE_NAMESPACE

Camera::Camera(const std::string_view aName) : Asset{ Asset::Type::CAMERA, aName } {}

std::unique_ptr<Camera> Camera::alloc(std::string_view aName)
{
	return std::make_unique<Camera>(aName); 
}

void Camera::initPerspectiveCamera(const float aYFov, const float aAspectRatio, const float aZNear, const float aZFar)
{
	this->mType = Type::PERSPECTIVE;
	this->mAspectRatio = aAspectRatio;
	this->mYFov = aYFov;
	this->mZFar = aZFar;
	this->mZNear = aZNear;
	calculateProjectionMatrix();
}

void Camera::initOrthographicCamera(const float aXMag, const float aYMag, const float aZNear, const float aZFar)
{
	mType = Type::ORTHOGRAPHIC;
	mXMag = aXMag;
	mYMag = aYMag;
	mZFar = aZFar;
	mZNear = aZNear;
	calculateProjectionMatrix();
}

void Camera::updateAspectRatio(const float aAspectRatio)
{
	mAspectRatio = aAspectRatio;
}

void Camera::updateMag(const float aXMag, const float aYMag)
{
	mXMag = aXMag;
	mYMag = aYMag;
}

bool Camera::updateAspectRatio(const glm::uvec2 aSize)
{
	if(glm::any(glm::notEqual(mSize, aSize)))
	{
		mSize = aSize;
		mAspectRatio = static_cast<float>(aSize.x) / static_cast<float>(aSize.y);
		return true;
	}
	return false;
}

bool Camera::updateMag(const glm::uvec2 aSize)
{
	if (glm::any(glm::notEqual(mSize, aSize)))
	{
		mSize = aSize;
		mAspectRatio = (static_cast<float>(aSize.x) * 0.5f) / (static_cast<float>(aSize.y) * 0.5f);
		return true;
	}
	return false;
}

glm::mat4 Camera::getProjectionMatrix()
{
	if (mType == Type::PERSPECTIVE) {
        mProjectionMatrix[0][0] = 1.0f / (mAspectRatio * std::tan(0.5f * mYFov));
	}
	
	return glm::perspectiveFov(glm::radians(60.0f), static_cast<float>(mSize.x), static_cast<float>(mSize.y), mZNear, mZFar);
}

Camera::Type Camera::getType() const
{
	return mType;
}

float Camera::getZFar() const
{
	return mZFar;
}

float Camera::getZNear() const
{
	return mZNear;
}

float Camera::getAspectRation() const
{
	return mAspectRatio;
}

float Camera::getYFov() const
{
	return mYFov;
}

float Camera::getXMag() const
{
	return mXMag;
}

float Camera::getYMag() const
{
	return mYMag;
}

void Camera::calculateProjectionMatrix()
{
	if (mType == Type::PERSPECTIVE) {
		const bool infinite = (mZFar == 0.0f);
		if (!infinite) this->mProjectionMatrix = glm::perspective(mYFov, mAspectRatio, mZNear, mZFar);
		else mProjectionMatrix = glm::infinitePerspective(mYFov, mAspectRatio, mZNear);
	}
	else if (mType == Type::ORTHOGRAPHIC) {
		constexpr float zero = 0;
		mProjectionMatrix = glm::ortho(zero, mXMag, zero, mYMag, mZNear, mZFar);
	}
}

glm::mat4 Camera::flipY(glm::mat4 aViewMatrix)
{
	constexpr glm::mat4 flipMatrix = {
	 1,   0,  0,  0,
	 0,  -1,  0,  0,
	 0,   0,  1,  0,
	 0,   0,  0,  1
	};
	return aViewMatrix = flipMatrix * aViewMatrix;
}


void Camera::buildViewMatrix(float* aViewMatrix, glm::vec3 const& aPosition, glm::vec3 const& aRight, glm::vec3 const& aUp, glm::vec3 const& aForward) {
	
	aViewMatrix[0] = aRight[0];
	aViewMatrix[4] = aRight[1];
	aViewMatrix[8] = aRight[2];
	aViewMatrix[12] = (-aPosition[0] * aRight[0] + -aPosition[1] * aRight[1] + -aPosition[2] * aRight[2]);
	
	aViewMatrix[1] = aUp[0];
	aViewMatrix[5] = aUp[1];
	aViewMatrix[9] = aUp[2];
	aViewMatrix[13] = (-aPosition[0] * aUp[0] + -aPosition[1] * aUp[1] + -aPosition[2] * aUp[2]);
	
	aViewMatrix[2] = aForward[0];
	aViewMatrix[6] = aForward[1];
	aViewMatrix[10] = aForward[2];
	aViewMatrix[14] = (-aPosition[0] * aForward[0] + -aPosition[1] * aForward[1] + -aPosition[2] * aForward[2]);
	
	aViewMatrix[3] = 0;
	aViewMatrix[7] = 0;
	aViewMatrix[11] = 0;
	aViewMatrix[15] = 1;
}

void Camera::buildModelMatrix(float* aModelMatrix, glm::vec3 const& aPosition, glm::vec3 const& aRight,
	glm::vec3 const& aUp, glm::vec3 const& aForward)
{
	aModelMatrix[0] = aRight[0]; aModelMatrix[1] = aRight[1]; aModelMatrix[2] = aRight[2]; aModelMatrix[3] = 0;
	aModelMatrix[4] = aUp[0]; aModelMatrix[5] = aUp[1]; aModelMatrix[6] = aUp[2]; aModelMatrix[7] = 0;
	aModelMatrix[8] = aForward[0]; aModelMatrix[9] = aForward[1]; aModelMatrix[10] = aForward[2]; aModelMatrix[11] = 0;
	aModelMatrix[12] = aPosition[0]; aModelMatrix[13] = aPosition[1]; aModelMatrix[14] = aPosition[2]; aModelMatrix[15] = 1;
}
