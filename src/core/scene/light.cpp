#include <tamashii/core/scene/light.hpp>

T_USE_NAMESPACE

Light::Type Light::getType() const
{
	return mLightType;
}

glm::vec3 Light::getColor() const
{
	return mColor;
}

float Light::getIntensity() const
{
	return mIntensity;
}

glm::vec4 Light::getDefaultDirection() const
{
	return { mDirection, 0 };
}

glm::vec4 Light::getDefaultTangent() const
{
	return { mTangent, 0 };
}

void Light::setColor(const glm::vec3 aColor)
{
	mColor = aColor;
}

void Light::setIntensity(const float aIntensity)
{
	mIntensity = aIntensity;
}

void Light::setDefaultDirection(const glm::vec3 aDir)
{
	mDirection = aDir;
}

void Light::setDefaultTangent(const glm::vec3 aDir)
{
	mTangent = aDir;
}

Light::Light(const Type aLightType, const glm::vec3 aDirection, const glm::vec3 aTangent) :
	Asset(Asset::Type::LIGHT), mLightType(aLightType), mColor({ 1.0f,1.0f,1.0f }),
	mIntensity(1.0f), mDirection(aDirection), mTangent(aTangent) {}

PointLight::PointLight() : Light(Light::Type::POINT, { 0,0,-1 }, { 1,0,0 }), mRange(0.0f), mRadius(0.0f) {}

Light_s	PointLight::getRawData() const {
	Light_s l = {};
	l.type = static_cast<uint32_t>(LightType::POINT);
	l.color = mColor;
	l.intensity = mIntensity;
	l.range = mRange;
	l.light_offset = mRadius;
	return l;
}

float PointLight::getRange() const
{
	return mRange;
}

float PointLight::getRadius() const
{
	return mRadius;
}

void PointLight::setRange(const float aRange)
{
	mRange = aRange;
}

void PointLight::setRadius(const float aRadius)
{
	mRadius = aRadius;
}

SpotLight::SpotLight(const float aInnerConeAngle, const float aOuterConeAngle) :
	Light(Type::SPOT, { 0,0,-1 }, { 1,0,0 }),
	mRange(0.0f), mRadius(0.0f), mCone({ aInnerConeAngle, aOuterConeAngle }) {}

Light_s SpotLight::getRawData() const
{
	Light_s l = {};
	l.type = static_cast<uint32_t>(LightType::SPOT);
	l.color = mColor;
	l.intensity = mIntensity;
	l.range = mRange;
	l.light_offset = mRadius;
	l.inner_angle = mCone.inner_angle;
	l.outer_angle = mCone.outer_angle;
    l.light_angle_scale = 1.0f / std::fmax(0.001f, (cos(mCone.inner_angle) - cos(mCone.outer_angle)));
	l.light_angle_offset = -cos(l.outer_angle) * l.light_angle_scale;
	return l;
}

float SpotLight::getRange() const
{
	return mRange;
}

float SpotLight::getRadius() const
{
	return mRadius;
}

float SpotLight::getInnerConeAngle() const
{
	return mCone.inner_angle;
}

float SpotLight::getOuterConeAngle() const
{
	return mCone.outer_angle;
}

void SpotLight::setRange(const float aRange)
{
	mRange = aRange;
}

void SpotLight::setRadius(const float aRadius)
{
	mRadius = aRadius;
}

void SpotLight::setCone(const float aInnerAngle, const float aOuterAngle)
{
	mCone.inner_angle = aInnerAngle;
	mCone.outer_angle = aOuterAngle;
}

DirectionalLight::DirectionalLight() : Light(Type::DIRECTIONAL, { 0,0,-1 }, { 1,0,0 }), mAngle(0.0f) {}

Light_s DirectionalLight::getRawData() const
{
	Light_s l = {};
	l.type = static_cast<uint32_t>(LightType::DIRECTIONAL);
	l.color = mColor;
	l.intensity = mIntensity;
	l.light_offset = mAngle;
	return l;
}

float DirectionalLight::getAngle() const
{
	return mAngle;
}

void DirectionalLight::setAngle(const float aAngle)
{
	mAngle = aAngle;
}

Light_s SurfaceLight::getRawData() const
{
	Light_s l = {};
	switch (mShape) {
	case Shape::SQUARE: l.type = static_cast<uint32_t>(LightType::SQUARE); break;
	case Shape::RECTANGLE: l.type = static_cast<uint32_t>(LightType::RECTANGLE); break;
	case Shape::CUBE: l.type = static_cast<uint32_t>(LightType::CUBE); break;
	case Shape::CUBOID: l.type = static_cast<uint32_t>(LightType::CUBOID); break;
	case Shape::DISK: l.type = static_cast<uint32_t>(LightType::DISK); break;
	case Shape::ELLIPSE: l.type = static_cast<uint32_t>(LightType::ELLIPSE); break;
	case Shape::SPHERE: l.type = static_cast<uint32_t>(LightType::SPHERE); break;
	case Shape::ELLIPSOID: l.type = static_cast<uint32_t>(LightType::ELLIPSOID); break;
	}
	l.color = mColor;
	l.intensity = mIntensity;
	l.dimensions = mDimension;
	l.double_sided = mDoubleSided;
	return l;
}

glm::vec4 SurfaceLight::getCenter() const
{
	return { 0,0,0,1 };
}

SurfaceLight::Shape SurfaceLight::getShape() const
{
	return mShape;
}

void SurfaceLight::setShape(const Shape aShape)
{
	mShape = aShape;
	switch (mShape) {
	case Shape::SQUARE:
	case Shape::RECTANGLE:
	case Shape::DISK:
	case Shape::ELLIPSE:
		mDimension = glm::vec3(1.0f, 1.0f, 0.0f);
		break;
	case Shape::CUBE:
	case Shape::CUBOID:
	case Shape::SPHERE:
	case Shape::ELLIPSOID:
		mDimension = glm::vec3(1.0f, 1.0f, 1.0f);
		break;
	}
}

glm::vec3 SurfaceLight::getDimensions() const
{
	return mDimension;
}

void SurfaceLight::setDimensions(const glm::vec3 aDimension)
{
	mDimension = aDimension;
	if (!is3D()) mDimension.z = 0;
}

bool SurfaceLight::is3D() const
{
	if (mShape == Shape::CUBE || mShape == Shape::CUBOID ||
		mShape == Shape::SPHERE || mShape == Shape::ELLIPSOID) return true;
	else return false;
}

bool SurfaceLight::doubleSided() const
{
	return mDoubleSided;
}

void SurfaceLight::doubleSided(bool aDoubleSided)
{
	mDoubleSided = aDoubleSided;
}

IESLight::IESLight() : Light(Light::Type::IES, { 0,0,-1 }, { 1,0,0 }), mRadius(0.0f), mCandelaTexture(nullptr) {}

Light_s IESLight::getRawData() const
{
	Light_s l = {};
	l.type = static_cast<uint32_t>(LightType::IES);
	l.color = mColor;
	l.intensity = mIntensity;
	l.min_vertical_angle = mVerticalAngles.empty() ? 0 : mVerticalAngles.front();
	l.max_vertical_angle = mVerticalAngles.empty() ? 0 : mVerticalAngles.back();
	l.min_horizontal_angle = mHorizontalAngles.empty() ? 0 : mHorizontalAngles.front();
	l.max_horizontal_angle = mHorizontalAngles.empty() ? 0 : mHorizontalAngles.back();
	l.texture_index = -1;
	l.light_offset = mRadius;
	return l;
}

float IESLight::getRadius() const
{
	return mRadius;
}

void IESLight::setRadius(float aRadius)
{
	mRadius = aRadius;
}

float IESLight::getMinVerticalAngle() const
{
	return mVerticalAngles.empty() ? 0 : mVerticalAngles.front();
}

float IESLight::getMaxVerticalAngle() const
{
	return mVerticalAngles.empty() ? 0 : mVerticalAngles.back();
}

float IESLight::getMinHorizontalAngle() const
{
	return mHorizontalAngles.empty() ? 0 : mHorizontalAngles.front();
}

float IESLight::getMaxHorizontalAngle() const
{
	return mHorizontalAngles.empty() ? 0 : mHorizontalAngles.back();
}

void IESLight::setVerticalAngles(const std::vector<float>& aVerticalAngles)
{
	mVerticalAngles = aVerticalAngles;
}

void IESLight::setHorizontalAngles(const std::vector<float>& aHorizontalAngles)
{
	mHorizontalAngles = aHorizontalAngles;
}

Texture* IESLight::getCandelaTexture() const
{
	return mCandelaTexture;
}

void IESLight::setCandelaTexture(Texture* aCandelaTexture)
{
	mCandelaTexture = aCandelaTexture;
}
