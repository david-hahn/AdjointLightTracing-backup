#define HLSL
#include "defines.h"


#define CONV_TEXTURE_BINDING BINDLESS_IMAGE_DESC_BINDING
#define CONV_TEXTURE_SET BINDLESS_IMAGE_DESC_SET
#include "../convenience/hlsl/texture_data.hlsl"

#define CONV_MATERIAL_BUFFER_BINDING ADJOINT_DESC_MATERIAL_BUFFER_BINDING
#define CONV_MATERIAL_BUFFER_SET ADJOINT_DESC_SET
#include "../convenience/hlsl/material_data.hlsl"

#define CONV_INDEX_BUFFER_BINDING ADJOINT_DESC_INDEX_BUFFER_BINDING
#define CONV_INDEX_BUFFER_SET ADJOINT_DESC_SET
#define CONV_VERTEX_BUFFER_BINDING ADJOINT_DESC_VERTEX_BUFFER_BINDING
#define CONV_VERTEX_BUFFER_SET ADJOINT_DESC_SET
#include "../convenience/hlsl/vertex_data.hlsl"

#define CONV_TLAS_BINDING ADJOINT_DESC_TLAS_BINDING
#define CONV_TLAS_SET ADJOINT_DESC_SET
#define CONV_GEOMETRY_BUFFER_BINDING ADJOINT_DESC_GEOMETRY_BUFFER_BINDING
#define CONV_GEOMETRY_BUFFER_SET ADJOINT_DESC_SET
#include "../convenience/hlsl/as_data.hlsl"

[[vk::binding(ADJOINT_DESC_INFO_BUFFER_BINDING, ADJOINT_DESC_SET)]] StructuredBuffer<AdjointInfo_s> info;
[[vk::binding(ADJOINT_DESC_RADIANCE_BUFFER_BINDING, ADJOINT_DESC_SET)]] RWStructuredBuffer<float> radiance;



[shader("raygeneration")]
void main()
{
	uint3 launchIndex = DispatchRaysIndex();
	uint3 launchDim = DispatchRaysDimensions();
	const uint entries_per_vertex = info[0].entries_per_vertex;

	
	RayDesc rayDesc;
	rayDesc.Origin = float3(0, 0, 0);
	rayDesc.Direction = float3(1, 1, 1);
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 100.0;

	AdjointPayload ap;
	
	TraceRay(tlas, RAY_FLAG_NONE, 0xff, 0, 0, 0, rayDesc, ap);
    if ( /*MISS*/ ap.instanceIndex == -1) return;

	const float3 barycentricCoords = float3(1.0f - ap.hitAttribute.x - ap.hitAttribute.y, ap.hitAttribute.x, ap.hitAttribute.y);
	const GeometryData_s geometry_data = geometry_buffer[ap.customInstanceID + ap.geometryIndex];
	const Material_s material = material_buffer[geometry_data.material_index];
	
	
    uint idx_0 = (ap.primitiveIndex * 3) + 0;
    uint idx_1 = (ap.primitiveIndex * 3) + 1;
    uint idx_2 = (ap.primitiveIndex * 3) + 2;
    if (geometry_data.has_indices > 0)
    {
        idx_0 = index_buffer[geometry_data.index_buffer_offset + idx_0];
        idx_1 = index_buffer[geometry_data.index_buffer_offset + idx_1];
        idx_2 = index_buffer[geometry_data.index_buffer_offset + idx_2];
    }
    idx_0 *= entries_per_vertex;
    idx_1 *= entries_per_vertex;
    idx_2 *= entries_per_vertex;
	
    const uint rad_buf_idx_0 = (geometry_data.vertex_buffer_offset + idx_0) * entries_per_vertex;
    const uint rad_buf_idx_1 = (geometry_data.vertex_buffer_offset + idx_1) * entries_per_vertex;
    const uint rad_buf_idx_2 = (geometry_data.vertex_buffer_offset + idx_2) * entries_per_vertex;
	
    for (int i = 0; i < entries_per_vertex; i++)
    {
        
        
        
    }
}