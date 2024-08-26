#version 460 
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

layout (points) in;
layout (line_strip, max_vertices = 8) out;

layout(location = 0) in vec4 p_center[];
layout(location = 1) in vec4 p_normal[];
layout(location = 2) in vec4 p_tangent[];
layout(location = 3) in vec4 p_bitangent[];

layout(binding = 0, set = 0) uniform global_ubo { GlobalUbo ubo; };

layout (location = 0) out vec4 color;

void main() {
    const vec4 n_color = ubo.normal_color;
    const vec4 t_color = ubo.tangent_color;
    const vec4 b_color = ubo.bitangent_color;
    const mat4 vpMat = ubo.projMat * ubo.viewMat;
    const vec4 center = vpMat * p_center[0];
    
    gl_Position = center; 
    color = n_color;
    EmitVertex();
    gl_Position = vpMat * p_normal[0];
    color = n_color;
    EmitVertex();

    gl_Position = center;
    color = vec4(0);
    EmitVertex();

    
    gl_Position = center; 
    color = t_color;
    EmitVertex();
    gl_Position = vpMat * p_tangent[0];
    color = t_color;
    EmitVertex();

    gl_Position = center;
    color = vec4(0); 
    EmitVertex();

    
    gl_Position = center; 
    color = b_color;
    EmitVertex();
    gl_Position = vpMat * p_bitangent[0];
    color = b_color;
    EmitVertex();
    
    EndPrimitive();
}