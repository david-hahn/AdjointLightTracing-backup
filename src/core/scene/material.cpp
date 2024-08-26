#include <tamashii/core/scene/material.hpp>
#include <tamashii/core/scene/image.hpp>
T_USE_NAMESPACE

Material::Material(const std::string_view aName) : Asset{ Asset::Type::MATERIAL, aName }, mBlendMode{ BlendMode::_OPAQUE }, mAlphaDiscardValue{ 0.5f }, mCullBackface{ false },
                                                   
                                                   mBaseColorFactor{1.0f},
                                                   mMetallicFactor{0.0f}, mRoughnessFactor{0.5f},
                                                   mEmissionFactor{0.0f}, mEmissionStrength{1.0f},
                                                   mNormalScale{1.0f},
                                                   mOcclusionStrength{1.0f},
                                                   mSpecularFactor{1.0f}, mSpecularColorFactor{ 0.04f,0.04f,0.04f },
                                                   mTransmissionFactor{0.0f},
                                                   mIor{1.5f},
                                                   mThicknessFactor{0.0f},
                                                   mAttenuationDistance{0.0f},
                                                   mAttenuationColor{1.0f},
                                                   mAttenuationAnisotropy{0.0f},
                                                   mLightFactor{1.0f},
                                                   
                                                   mBaseColorTexture{nullptr},
                                                   mMetallicTexture{nullptr}, mRoughnessTexture{nullptr},
                                                   mEmissionTexture{nullptr},
                                                   mNormalTexture{nullptr}, 
                                                   mOcclusionTexture{nullptr},
                                                   mSpecularTexture{nullptr}, mSpecularColorTexture{nullptr},
                                                   mTransmissionTexture{nullptr},
                                                   mThicknessTexture{nullptr},
                                                   mLightTexture{nullptr},
                                                   mCustomTexture{nullptr}
{}

Material* Material::alloc(const std::string_view aName)
{ return new Material(aName); }

void Material::setCullBackface(const bool aCull)
{ mCullBackface = aCull; }

void Material::setBlendMode(const BlendMode aBlendMode)
{ mBlendMode = aBlendMode; }

void Material::setAlphaDiscardValue(const float aAlphaDiscardValue)
{ mAlphaDiscardValue = aAlphaDiscardValue; }

bool Material::getCullBackface() const
{ return mCullBackface; }

Material::BlendMode Material::getBlendMode() const
{ return mBlendMode; }

float Material::getAlphaDiscardValue() const
{ return mAlphaDiscardValue; }

bool Material::hasBaseColorTexture() const
{ return mBaseColorTexture != nullptr; }

bool Material::hasMetallicTexture() const
{ return mMetallicTexture != nullptr; }

bool Material::hasRoughnessTexture() const
{ return mRoughnessTexture != nullptr; }

bool Material::hasEmissionTexture() const
{ return mEmissionTexture != nullptr; }

bool Material::hasNormalTexture() const
{ return mNormalTexture != nullptr; }

bool Material::hasOcclusionTexture() const
{ return mOcclusionTexture != nullptr; }

bool Material::hasSpecularTexture() const
{ return mSpecularTexture != nullptr; }

bool Material::hasSpecularColorTexture() const
{ return mSpecularColorTexture != nullptr; }

glm::vec4 Material::getBaseColorFactor() const
{ return mBaseColorFactor; }

Texture* Material::getBaseColorTexture() const
{ return mBaseColorTexture; }

void Material::setBaseColorFactor(const glm::vec4& aVec)
{ mBaseColorFactor = aVec; }

void Material::setBaseColorTexture(Texture* aTexture)
{ mBaseColorTexture = aTexture; }

float Material::getMetallicFactor() const
{ return mMetallicFactor; }

Texture* Material::getMetallicTexture() const
{ return mMetallicTexture; }

void Material::setMetallicFactor(const float& aFloat)
{ mMetallicFactor = aFloat; }

void Material::setMetallicTexture(Texture* aTexture)
{ mMetallicTexture = aTexture; }

float Material::getRoughnessFactor() const
{ return mRoughnessFactor; }

Texture* Material::getRoughnessTexture() const
{ return mRoughnessTexture; }

void Material::setRoughnessFactor(const float& aFloat)
{ mRoughnessFactor = aFloat; }

void Material::setRoughnessTexture(Texture* aTexture)
{ mRoughnessTexture = aTexture; }

bool Material::isLight() const
{ return mEmissionStrength != 0.0f && glm::any(glm::notEqual(mEmissionFactor, glm::vec3(0.0f))); }

float Material::getEmissionStrength() const
{ return mEmissionStrength; }

glm::vec3 Material::getEmissionFactor() const
{ return mEmissionFactor; }

Texture* Material::getEmissionTexture() const
{ return mEmissionTexture; }

void Material::setEmissionStrength(const float& aFloat)
{ mEmissionStrength = aFloat; }

void Material::setEmissionFactor(const glm::vec3& aVec)
{ mEmissionFactor = aVec; }

void Material::setEmissionTexture(Texture* aTexture)
{ mEmissionTexture = aTexture; }

float Material::getNormalScale() const
{ return mNormalScale; }

Texture* Material::getNormalTexture() const
{ return mNormalTexture; }

void Material::setNormalScale(const float& aFloat)
{ mNormalScale = aFloat; }

void Material::setNormalTexture(Texture* aTexture)
{ mNormalTexture = aTexture; }

float Material::getOcclusionStrength() const
{ return mOcclusionStrength; }

Texture* Material::getOcclusionTexture() const
{ return mOcclusionTexture; }

void Material::setOcclusionStrength(const float& aFloat)
{ mOcclusionStrength = aFloat; }

void Material::setOcclusionTexture(Texture* aTexture)
{ mOcclusionTexture = aTexture; }

float Material::getSpecularFactor() const
{ return mSpecularFactor; }

glm::vec3 Material::getSpecularColorFactor() const
{ return mSpecularColorFactor; }

Texture* Material::getSpecularTexture() const
{ return mSpecularTexture; }

Texture* Material::getSpecularColorTexture() const
{ return mSpecularColorTexture; }

void Material::setSpecularFactor(const float& aFloat)
{ mSpecularFactor = aFloat; }

void Material::setSpecularColorFactor(const glm::vec3& aVec)
{ mSpecularColorFactor = aVec; }

void Material::setSpecularTexture(Texture* aTexture)
{ mSpecularTexture = aTexture; }

void Material::setSpecularColorTexture(Texture* aTexture)
{ mSpecularColorTexture = aTexture; }

float Material::getTransmissionFactor() const
{ return mTransmissionFactor; }

Texture* Material::getTransmissionTexture() const
{ return mTransmissionTexture; }

void Material::setTransmissionFactor(const float& aFloat)
{ mTransmissionFactor = aFloat; }

void Material::setTransmissionTexture(Texture* aTexture)
{ mTransmissionTexture = aTexture; }

float Material::getThicknessFactor() const
{ return mThicknessFactor; }

Texture* Material::getThicknessTexture() const
{ return mThicknessTexture; }

float Material::getAttenuationDistance() const
{ return mAttenuationDistance; }

glm::vec3 Material::getAttenuationColor() const
{ return mAttenuationColor; }

void Material::setThicknessFactor(const float& aFloat)
{ mThicknessFactor = aFloat; }

void Material::setThicknessTexture(Texture* aTexture)
{ mThicknessTexture = aTexture; }

void Material::setAttenuationDistance(const float& aFloat)
{ mAttenuationDistance = aFloat; }

void Material::setAttenuationColor(const glm::vec3& aVec)
{ mAttenuationColor = aVec; }

void Material::setAttenuationAnisotropy(const float& aFloat)
{ mAttenuationAnisotropy = aFloat; }

float Material::getAttenuationAnisotropy() const
{ return mAttenuationAnisotropy; }

float Material::getIOR() const
{ return mIor; }

void Material::setIOR(const float& aFloat)
{ mIor = aFloat; }

glm::vec3 Material::getLightFactor() const
{ return mLightFactor; }

void Material::setLightFactor(const glm::vec3& aVec)
{ mLightFactor = aVec; }

Texture* Material::getLightTexture() const
{ return mLightTexture; }

void Material::setLightTexture(Texture* aTexture)
{ mLightTexture = aTexture; }

Texture* Material::getCustomTexture() const
{ return mCustomTexture; }

void Material::setCustomTexture(Texture* aTexture)
{ mCustomTexture = aTexture; }

Material::~Material() = default;

Material::Material(const Material& aMaterial) : Asset{ aMaterial.getAssetType(), aMaterial.mName }, mBlendMode{aMaterial.mBlendMode},
                                        mAlphaDiscardValue{aMaterial.mAlphaDiscardValue}, mCullBackface{aMaterial.mCullBackface},
										
                                        mBaseColorFactor{aMaterial.mBaseColorFactor}, mMetallicFactor{aMaterial.mMetallicFactor},
                                        mRoughnessFactor{aMaterial.mRoughnessFactor}, mEmissionFactor{aMaterial.mEmissionFactor},
                                        mEmissionStrength{aMaterial.mEmissionStrength}, mNormalScale{aMaterial.mNormalScale},
                                        mOcclusionStrength{aMaterial.mOcclusionStrength}, mSpecularFactor{aMaterial.mSpecularFactor},
                                        mSpecularColorFactor{aMaterial.mSpecularColorFactor},
                                        mTransmissionFactor{aMaterial.mTransmissionFactor}, mIor{aMaterial.mIor},
                                        mThicknessFactor{aMaterial.mThicknessFactor},
                                        mAttenuationDistance{aMaterial.mAttenuationDistance}, mAttenuationColor{aMaterial.mAttenuationColor},
										mAttenuationAnisotropy{aMaterial.mAttenuationAnisotropy},
										mLightFactor{aMaterial.mLightFactor},
										
										mBaseColorTexture{aMaterial.mBaseColorTexture}, mMetallicTexture{aMaterial.mMetallicTexture},
										mRoughnessTexture{aMaterial.mRoughnessTexture}, mEmissionTexture{aMaterial.mEmissionTexture},
                                        mNormalTexture{aMaterial.mNormalTexture}, mOcclusionTexture{aMaterial.mOcclusionTexture},
                                        mSpecularTexture{aMaterial.mSpecularTexture},
                                        mSpecularColorTexture{aMaterial.mSpecularColorTexture},
                                        mTransmissionTexture{aMaterial.mTransmissionTexture},
                                        mThicknessTexture{aMaterial.mThicknessTexture}, mLightTexture{aMaterial.mLightTexture}, mCustomTexture{aMaterial.mCustomTexture} {}

Material_s Material::getRawData() const
{
	Material_s material = {};
	
	
	
	material.alphaDiscardValue = mAlphaDiscardValue;
	if(mBlendMode == BlendMode::MASK) material.alphaDiscard = true;
	
	material.baseColorTexIdx = -1;
	material.metallicTexIdx = -1;
	material.roughnessTexIdx = -1;
	material.occlusionTexIdx = -1;
	material.normalTexIdx = -1;
	material.emissionTexIdx = -1;
	material.specularTexIdx = -1;
	material.specularColorTexIdx = -1;
	material.transmissionTexIdx = -1;
	material.lightTexIdx = -1;
	
	material.clearcoatTexIdx = -1;
	material.clearcoatRoughnessTexIdx = -1;
	material.clearcoatNormalTexIdx = -1;
	
	material.sheenColorTexIdx = -1;
	material.sheenRoughnessTexIdx = -1;
	material.customTexIdx = -1;

	
	material.baseColorTexCoordIdx = mBaseColorTexture != nullptr ? mBaseColorTexture->texCoordIndex : -1;
	material.metallicTexCoordIdx = mMetallicTexture != nullptr ? mMetallicTexture->texCoordIndex : -1;
	material.roughnessTexCoordIdx = mRoughnessTexture != nullptr ? mRoughnessTexture->texCoordIndex : -1;
	material.occlusionTexCoordIdx = mOcclusionTexture != nullptr ? mOcclusionTexture->texCoordIndex : -1;
	material.normalTexCoordIdx = mNormalTexture != nullptr ? mNormalTexture->texCoordIndex : -1;
	material.emissionTexCoordIdx = mEmissionTexture != nullptr ? mEmissionTexture->texCoordIndex : -1;
	material.specularTexCoordIdx = mSpecularTexture != nullptr ? mSpecularTexture->texCoordIndex : -1;
	material.specularColorTexCoordIdx = mSpecularColorTexture != nullptr ? mSpecularColorTexture->texCoordIndex : -1;
	material.transmissionTexCoordIdx = mTransmissionTexture != nullptr ? mTransmissionTexture->texCoordIndex : -1;
	material.lightTexCoordIdx = mLightTexture != nullptr ? mLightTexture->texCoordIndex : -1;
	
	material.clearcoatTexCoordIdx = -1;
	material.clearcoatRoughnessTexCoordIdx = -1;
	material.clearcoatNormalTexCoordIdx = -1;
	
	material.sheenColorTexCoordIdx = -1;
	material.sheenRoughnessTexCoordIdx = -1;
	material.customTexCoordIdx = mCustomTexture != nullptr ? mCustomTexture->texCoordIndex : -1;

	
	material.baseColorFactor = mBaseColorFactor;
	material.metallicFactor = mMetallicFactor;
	material.roughnessFactor = mRoughnessFactor;
	material.occlusionStrength = mOcclusionStrength;
	material.normalScale = mNormalScale;
	material.emissionFactor = mEmissionFactor;
	material.emissionStrength = mEmissionStrength;
	material.specularColorFactor = mSpecularColorFactor;
	material.specularFactor = mSpecularFactor;
	material.ior = mIor;
	material.transmissionFactor = mTransmissionFactor;
	material.clearcoatFactor = 0;
	material.clearcoatRoughnessFactor = 0;
	material.sheenColorFactor = glm::vec3(0);
	material.sheenRoughnessFactor = 0;
	material.attenuationColor = mAttenuationColor;
	material.attenuationDistance = mAttenuationDistance;
	material.attenuationAnisotropy = mAttenuationAnisotropy;
	material.lightFactor = mLightFactor;
	material.isEmissive = isLight();
	material.isDoubleSided = !mCullBackface;

	return material;
}

