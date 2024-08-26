#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/scene/asset.hpp>
#include <tamashii/core/scene/image.hpp>
#include <array>

T_BEGIN_NAMESPACE
enum class LightType : uint32_t {
	
	POINT							= 0x00000001,
	SPOT							= 0x00000002,
	DIRECTIONAL						= 0x00000004,
	SQUARE							= 0x00000008,
	RECTANGLE						= 0x00000010,
	CUBE							= 0x00000020,
	CUBOID							= 0x00000040,
	DISK							= 0x00000080,
	ELLIPSE							= 0x00000100,
	SPHERE							= 0x00000200,
	ELLIPSOID						= 0x00000400,
	IES								= 0x00000800,
	TRIANGLE_MESH					= 0x00001000,
	
	PUNCTUAL						= (POINT | SPOT | DIRECTIONAL),
	HYPERRECTANGULAR				= (SQUARE | RECTANGLE | CUBE | CUBOID),
	ELLIPTICAL						= (DISK | ELLIPSE | SPHERE | ELLIPSOID),
	SURFACE							= (HYPERRECTANGULAR | ELLIPTICAL)
};


struct Light_s {
	
	glm::vec3						color;
	float							intensity;
	
	
	
	
	glm::vec4						pos_ws;
	
	
	
	
	glm::vec4						n_ws_norm;
	glm::vec4						t_ws_norm;
	
	
	glm::vec3						dimensions;
	uint32_t						double_sided;
	
	
	float							range;
	
	float							light_offset;		
	
	float							inner_angle;
	float							outer_angle;
	float							light_angle_scale;
	float							light_angle_offset;
	
	float							min_vertical_angle;
	float							max_vertical_angle;
	float							min_horizontal_angle;
	float							max_horizontal_angle;
	
	
	int								texture_index; 
	
	uint32_t						triangle_count;
	int								id;
	uint32_t						index_buffer_offset;	
	uint32_t						vertex_buffer_offset;
	
	uint32_t						type;
};

class Light : public Asset {
public:
	enum class Type
	{
		DIRECTIONAL, POINT, SPOT, SURFACE, IES
	};

	Type										getType() const;
	glm::vec3									getColor() const;
	float										getIntensity() const;
	glm::vec4									getDefaultDirection() const;
	glm::vec4									getDefaultTangent() const;

	void										setColor(glm::vec3 aColor);
	void										setIntensity(float aIntensity);
	void										setDefaultDirection(glm::vec3 aDir);
	void										setDefaultTangent(glm::vec3 aDir);

	virtual Light_s								getRawData() const = 0;
												~Light() override = default;
protected:
												Light(Type aLightType, glm::vec3 aDirection, glm::vec3 aTangent);

	Type										mLightType;

	glm::vec3									mColor;
												
												
	float										mIntensity;
												
	glm::vec3									mDirection;
	glm::vec3									mTangent;
};

class PointLight final : public Light {
public:
												PointLight();
												~PointLight() override = default;

	Light_s										getRawData() const override;
	float										getRange() const;
	float										getRadius() const;
	void										setRange(float aRange);
	void										setRadius(float aRadius);
private:
	float										mRange;						
	float										mRadius;
};

class SpotLight final : public Light {
public:
												SpotLight(float aInnerConeAngle = 0.0f, float aOuterConeAngle = 0.7853981634f/*PI/4*/);
												~SpotLight() override = default;

	Light_s										getRawData() const override;
	float										getRange() const;
	float										getRadius() const;
	float										getInnerConeAngle() const;
	float										getOuterConeAngle() const;
	void										setRange(float aRange);
	void										setRadius(float aRadius);
	void										setCone(float aInnerAngle, float aOuterAngle);
private:
	float										mRange;	
	float										mRadius;
	struct Cone {								
		float									inner_angle;				
		float									outer_angle; 
	} mCone;
};

class DirectionalLight final : public Light {
public:
												DirectionalLight();
												~DirectionalLight() override = default;

	Light_s										getRawData() const override;
	float										getAngle() const;
	void										setAngle(float aAngle);
private:
	float										mAngle; 
};

class SurfaceLight final : public Light {
public:
	enum class Shape
	{
		SQUARE, RECTANGLE, CUBE, CUBOID, DISK, ELLIPSE, SPHERE, ELLIPSOID
	};

												SurfaceLight() : Light(Type::SURFACE, { 0,0,-1 }, { 1,0,0 }), mShape(Shape::SQUARE), mDimension(1), mDoubleSided(false) {}
												~SurfaceLight() override = default;

	Light_s										getRawData() const override;
	glm::vec4									getCenter() const;

	Shape										getShape() const;
	void										setShape(Shape aShape);
	glm::vec3									getDimensions() const;
	void										setDimensions(glm::vec3 aDimension);
	bool										is3D() const;

	bool										doubleSided() const;
	void										doubleSided(bool aDoubleSided);
private:
	Shape										mShape;

	
	
	glm::vec3									mDimension;
	bool										mDoubleSided;
};

class IESLight final : public Light {
public:
												IESLight();
	Light_s										getRawData() const override;

	float										getRadius() const;
	void										setRadius(float aRadius);
	float										getMinVerticalAngle() const;
	float										getMaxVerticalAngle() const;
	float										getMinHorizontalAngle() const;
	float										getMaxHorizontalAngle() const;
	void										setVerticalAngles(const std::vector<float>& aVerticalAngles);
	void										setHorizontalAngles(const std::vector<float>& aHorizontalAngles);

	Texture*									getCandelaTexture() const;
	void										setCandelaTexture(Texture* aCandelaTexture);
private:
	float										mRadius;
	std::vector<float>							mVerticalAngles;
	std::vector<float>							mHorizontalAngles;

	Texture*									mCandelaTexture;
};

class ImageBasedLight : public Asset {
public:
												ImageBasedLight() : Asset(Asset::Type::LIGHT), mIntensity(1), mResolution(),
													mSampler({ Sampler::Filter::LINEAR, Sampler::Filter::LINEAR, 
														Sampler::Filter::LINEAR, Sampler::Wrap::CLAMP_TO_BORDER,
														Sampler::Wrap::CLAMP_TO_BORDER, Sampler::Wrap::CLAMP_TO_BORDER, 0, 1000 }),
													mImages()
												{}

	float										getIntensity() const;
	void										setIntensity(const float aIntensity) { mIntensity = aIntensity; }

	void										setCubeMap(const std::array<Image*, 6>& aImages)
												{
													mResolution = glm::vec2(aImages[0]->getWidth(), aImages[0]->getHeight());
													mImages = aImages;
												}
	std::array<Image*, 6>						getCubeMap() const { return mImages; }

	void										setSampler(const Sampler& aSampler) { mSampler = aSampler; }
	Sampler										getSampler() const { return mSampler; }
private:
	
	float										mIntensity;
	glm::vec2									mResolution;
	Sampler										mSampler;
	std::array<Image*, 6>						mImages;
};
T_END_NAMESPACE
