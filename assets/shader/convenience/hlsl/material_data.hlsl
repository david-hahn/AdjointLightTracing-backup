


#if defined(CONV_MATERIAL_BUFFER_BINDING) && defined(CONV_MATERIAL_BUFFER_SET)
#ifndef MATERIAL_DATA_HLSL
#define MATERIAL_DATA_HLSL

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
	int			clearcoatRoughnessTexture;
	int			clearcoatNormalTexture;
	/* -- 16 byte -- */
	int			sheenColorTexIdx;
	int			sheenRoughnessTexIdx;
	int			customTexIdx;
	
	int			baseColorTexCoordIdx;
	/* -- 16 byte -- */
	int			metallicTexCoordIdx;
	int			roughnessTexCoordIdx;
	int			occlusionTexCoordIdx;
	int			normalTexCoordIdx;
	/* -- 16 byte -- */
	int			emissionTexCoordIdx;
	int			specularTexCoordIdx;
	int			specularColorTexCoordIdx;
	int			transmissionTexCoordIdx;
	/* -- 16 byte -- */
	int			customTexCoordIdx;
	int			clearcoatTexCoordIdx;
	int			clearcoatRoughnessTexCoordIdx;
	int			clearcoatNormalTexCoordIdx;
	/* -- 16 byte -- */
	int			sheenColorTexCoordIdx;
	int			sheenRoughnessTexCoordIdx;
	int			_;		/*not set*/
	int			__;		/*not set*/
	/* -- 16 byte -- */
	
	float4		baseColorFactor;
	/* -- 16 byte -- */
	float		metallicFactor;
	float		roughnessFactor;
	float		occlusionStrength;
	float		normalScale;
	/* -- 16 byte -- */
	float3		emissionFactor;
	float		emissionStrength;
	/* -- 16 byte -- */
	float3		specularColorFactor;
	float		specularFactor;
	/* -- 16 byte -- */
	float		ior;
	float		transmissionFactor;
	float		clearcoatFactor;
	float		clearcoatRoughnessFactor;
	/* -- 16 byte -- */
	float3		sheenColorFactor;
	float		sheenRoughnessFactor;
	/* -- 16 byte -- */
	float		alphaDiscardValue;
	uint		isEmissive;
	int			___;		/*not set*/
	int			____;		/*not set*/
};
[[vk::binding(CONV_MATERIAL_BUFFER_BINDING, CONV_MATERIAL_BUFFER_SET)]] StructuredBuffer<Material_s> material_buffer;


#endif 	
#endif