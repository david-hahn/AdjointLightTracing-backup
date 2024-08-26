#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable
#include "defines.h"
#include "../utils/glsl/rendering_utils.glsl"




layout(location = 0) in vec3 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 uv0;
layout(location = 4) in vec2 uv1;
layout(location = 5) in vec4 color0;

layout(constant_id = 0) const uint SPHERICAL_HARMONIC_ORDER = 1;
layout(constant_id = 1) const uint ENTRIES_PER_VERTEX = 3;

layout(push_constant) uniform PushConstant{
    layout(offset = 0) mat4 model_matrix;
    layout(offset = 64) int material_idx;
};

layout(binding = RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict uniform global_ubo{ GlobalBufferR ubo; };
layout(binding = RASTERIZER_DESC_RADIANCE_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer radiance_storage_buffer { float radiance_buffer[]; };
layout(binding = RASTERIZER_DESC_VTX_COLOR_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer vtx_color_storage_buffer { float vtx_color_buffer[]; };

#define SH_READ
#define SH_DATA_BUFFER_ radiance_buffer
#include "../sphericalharmonics/sphericalharmonics.glsl"

layout(location = 0) out vec4 v_color0;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_tangent;
layout(location = 3) out vec2 v_uv0;
layout(location = 4) out uint out_radiance_buffer_offset;
layout(location = 5) out uint out_radiance_weights_buffer_offset;
layout(location = 8) out vec3 outPosWS;

void main() {
    gl_Position = ubo.projMat * ubo.viewMat * model_matrix * vec4(position.xyz, 1.0);

    outPosWS = vec3(model_matrix * vec4(position.xyz, 1.0));

    v_color0 = color0;
    v_uv0 = uv0;
    v_normal = normalize(mat3(transpose(inverse(model_matrix))) * normal.xyz);
    v_tangent = normalize(mat3(transpose(inverse(model_matrix))) * tangent.xyz);

    out_radiance_buffer_offset = gl_VertexIndex * ENTRIES_PER_VERTEX;
    out_radiance_weights_buffer_offset = gl_VertexIndex;

    if ( abs(dot(v_normal , v_normal )-1.0) > 1e-5 ) v_normal = normalize( v_normal );
    if ( abs(dot(v_tangent, v_tangent)-1.0) > 1e-5 || abs(dot(v_normal, v_tangent)) > 1e-5) v_tangent = normalize( getPerpendicularVector(v_normal) ); 
    vec3 viewDir = normalize(ubo.viewPos.xyz - outPosWS);
    vec3 rgb = evalHSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), int(out_radiance_buffer_offset), viewDir, v_normal, v_tangent);
    v_color0.rgb = rgb;
    
    
    
    
    
    
    
}
