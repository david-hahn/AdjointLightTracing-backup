




#ifndef AS_DATA_GLSL
#define AS_DATA_GLSL


#if defined(CONV_TLAS_BINDING) && defined(CONV_TLAS_SET)
layout(binding = CONV_TLAS_BINDING, set = CONV_TLAS_SET) uniform accelerationStructureEXT tlas;
#endif


#if defined(CONV_GEOMETRY_BUFFER_BINDING) && defined(CONV_GEOMETRY_BUFFER_SET)

const uint eGeoDataTriangleBit			= 0x00000001u;
const uint eGeoDataIndexedTriangleBit	= 0x00000003u;
const uint eGeoDataIntrinsicBit			= 0x00000004u;
const uint eGeoDataMaterialBit			= 0x00000008u;
const uint eGeoDataLightBit				= 0x00000010u;
struct GeometryData_s {
	mat4 model_matrix;
	uint index_buffer_offset;
	uint vertex_buffer_offset;
	uint data_index;
	uint flags;
};
layout(binding = CONV_GEOMETRY_BUFFER_BINDING, set = CONV_GEOMETRY_BUFFER_SET) buffer geometry_storage_buffer { GeometryData_s geometry_buffer[]; };
#endif

#endif 	