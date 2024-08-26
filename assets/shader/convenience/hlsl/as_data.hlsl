




#ifndef AS_DATA_HLSL
#define AS_DATA_HLSL


#if defined(CONV_TLAS_BINDING) && defined(CONV_TLAS_SET)
[[vk::binding(CONV_TLAS_BINDING, CONV_TLAS_SET)]] RaytracingAccelerationStructure tlas;
#endif


#if defined(CONV_GEOMETRY_BUFFER_BINDING) && defined(CONV_GEOMETRY_BUFFER_SET)
struct GeometryData_s {
	float4x4 model_matrix;
	uint index_buffer_offset;
	uint vertex_buffer_offset;
	uint has_indices;
	uint material_index;
};
[[vk::binding(CONV_GEOMETRY_BUFFER_BINDING, CONV_GEOMETRY_BUFFER_SET)]] StructuredBuffer<GeometryData_s> geometry_buffer;
#endif 

#endif 	