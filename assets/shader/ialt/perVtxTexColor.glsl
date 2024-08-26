#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_atomic_float : require
#extension GL_EXT_debug_printf : enable

#define GLSL

layout(push_constant) uniform PushConstant {
    layout(offset = 0) int geomIdx;
};

layout(binding = 0, set = 1, std430) buffer color_buffer { float vtxColors[]; };
layout(binding = 1, set = 1, std430) buffer area_buffer  { float vtxArea[]; };

#define CONV_INDEX_BUFFER_BINDING    2
#define CONV_VERTEX_BUFFER_BINDING   3
#define CONV_GEOMETRY_BUFFER_BINDING 4
#define CONV_MATERIAL_BUFFER_BINDING 5
#define CONV_TEXTURE_BINDING     0
#define CONV_TEXTURE_SET         0
#define CONV_INDEX_BUFFER_SET    1
#define CONV_VERTEX_BUFFER_SET   1
#define CONV_GEOMETRY_BUFFER_SET 1
#define CONV_MATERIAL_BUFFER_SET 1
#include "../convenience/glsl/vertex_data.glsl"
#include "../convenience/glsl/as_data.glsl"
#include "../convenience/glsl/texture_data.glsl"
#include "../convenience/glsl/material_data.glsl"

void main() {
    uint k = gl_GlobalInvocationID.x;

	
    const GeometryData_s geometry_data = geometry_buffer[geomIdx];
    if(k==0) debugPrintfEXT("geom %d", geomIdx);
    if(k==0) debugPrintfEXT("mat %d", geometry_data.data_index);

    const Material_s material = material_buffer[geometry_data.data_index];
    uint idx_0 = (k*3);
    uint idx_1 = (k*3) + 1;
    uint idx_2 = (k*3) + 2;
    if( (geometry_data.flags & eGeoDataIndexedTriangleBit) > 0){
        idx_0 = index_buffer[geometry_data.index_buffer_offset + idx_0];
        idx_1 = index_buffer[geometry_data.index_buffer_offset + idx_1];
        idx_2 = index_buffer[geometry_data.index_buffer_offset + idx_2];
        if(k==0) debugPrintfEXT("idx off %d", geometry_data.index_buffer_offset);
    }
    idx_0 += geometry_data.vertex_buffer_offset;
    idx_1 += geometry_data.vertex_buffer_offset;
    idx_2 += geometry_data.vertex_buffer_offset;
    if(k==0) debugPrintfEXT("vtx off %d", geometry_data.vertex_buffer_offset);

    vec3 v0 = (geometry_data.model_matrix * vertex_buffer[idx_0].position).xyz;
	vec3 v1 = (geometry_data.model_matrix * vertex_buffer[idx_1].position).xyz;
	vec3 v2 = (geometry_data.model_matrix * vertex_buffer[idx_2].position).xyz;
    float triArea = 1.0/2.0 * length(cross( v1-v0 , v2-v0 ));

    if( k==42 ) debugPrintfEXT("[%d]->(%d,%d,%d)", k, idx_0,idx_1,idx_2);
    vec3 bary=vec3(0.f);
    float weight = 1.0f/(128.0f*(128.0f+1.0f)/2.0f); 
    for(int xstep=0; xstep<128; ++xstep){
        for(int ystep=0; ystep<(128-xstep); ++ystep ){
            bary.x = (float(xstep)+1.0f/3.0f)/128.0f; 
            bary.y = (float(ystep)+1.0f/3.0f)/128.0f;
            bary.z=1.0f-bary.y-bary.x;
            vec3 texColor=material.baseColorFactor.xyz;
            {
		        vec2 uv_0 = vertex_buffer[idx_0].texture_coordinates_0.xy;
		        vec2 uv_1 = vertex_buffer[idx_1].texture_coordinates_0.xy;
		        vec2 uv_2 = vertex_buffer[idx_2].texture_coordinates_0.xy;
		        vec2 uv = (bary.x * uv_0 + bary.y * uv_1 + bary.z * uv_2);
                if(material.baseColorTexIdx != -1){
                    texColor *= (texture(texture_sampler[material.baseColorTexIdx], uv)).xyz;
                }
            }

            for(int i=0; i<3; ++i){ 
                atomicAdd(vtxColors[idx_0*3+i], texColor[i] * weight*bary[0]*triArea/vtxArea[idx_0]);
                atomicAdd(vtxColors[idx_1*3+i], texColor[i] * weight*bary[1]*triArea/vtxArea[idx_1]);
                atomicAdd(vtxColors[idx_2*3+i], texColor[i] * weight*bary[2]*triArea/vtxArea[idx_2]);
            }
        }
    }
}