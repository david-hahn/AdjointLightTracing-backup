


#if defined(CONV_MATERIAL_BUFFER_BINDING) && defined(CONV_MATERIAL_BUFFER_SET)
#ifndef MATERIAL_DATA_GLSL
#define MATERIAL_DATA_GLSL

struct Material_s {
	int			baseColorTexIdx;
	int			metallicTexIdx;
	int			roughnessTexIdx;
	int			occlusionTexIdx;
	/* -- 16 byte -- */
	int			normalTexIdx;
	int			emissionTexIdx;
	int			specularTexIdx;
	int			specularColorTexIdx;
	/* -- 16 byte -- */
	int			transmissionTexIdx;
	int			clearcoatTexIdx;
	int			clearcoatRoughnessTexIdx;
	int			clearcoatNormalTexIdx;
	/* -- 16 byte -- */
	int			sheenColorTexIdx;
	int			sheenRoughnessTexIdx;
	int			lightTexIdx;
	int			customTexIdx;
	/* -- 16 byte -- */
	
	int			baseColorTexCoordIdx;
	int			metallicTexCoordIdx;
	int			roughnessTexCoordIdx;
	int			occlusionTexCoordIdx;
	/* -- 16 byte -- */
	int			normalTexCoordIdx;
	int			emissionTexCoordIdx;
	int			specularTexCoordIdx;
	int			specularColorTexCoordIdx;
	/* -- 16 byte -- */
	int			transmissionTexCoordIdx;
	int			customTexCoordIdx;
	int			clearcoatTexCoordIdx;
	int			clearcoatRoughnessTexCoordIdx;
	/* -- 16 byte -- */
	int			clearcoatNormalTexCoordIdx;
	int			sheenColorTexCoordIdx;
	int			sheenRoughnessTexCoordIdx;
	int			lightTexCoordIdx;
	
	/* -- 16 byte -- */
	vec4		baseColorFactor;
	/* -- 16 byte -- */
	float		metallicFactor;
	float		roughnessFactor;
	float		occlusionStrength;
	float		normalScale;
	/* -- 16 byte -- */
	vec3		emissionFactor;
	float		emissionStrength;
	/* -- 16 byte -- */
	vec3		specularColorFactor;
	float		specularFactor;
	/* -- 16 byte -- */
	float		ior;
	float		transmissionFactor;
	float		clearcoatFactor;
	float		clearcoatRoughnessFactor;
	/* -- 16 byte -- */
	vec3		sheenColorFactor;
	float		sheenRoughnessFactor;
	/* -- 16 byte -- */
	vec3		lightFactor;
	float		attenuationAnisotropy;
	/* -- 16 byte -- */
	vec3		attenuationColor;
	float		attenuationDistance;
	/* -- 16 byte -- */
	float		alphaDiscardValue;
	bool		alphaDiscard;
	bool		isEmissive;
	bool		isDoubleSided;
};
layout(binding = CONV_MATERIAL_BUFFER_BINDING, set = CONV_MATERIAL_BUFFER_SET)  buffer material_storage_buffer { Material_s material_buffer[]; };

#endif 	
#endif