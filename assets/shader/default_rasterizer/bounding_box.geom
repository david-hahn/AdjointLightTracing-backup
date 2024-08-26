#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

layout (points) in;
layout (line_strip, max_vertices = 16) out;

layout(location = 0) in vec4 in_pos_0[];
layout(location = 1) in vec4 in_pos_1[];
layout(location = 2) in vec4 in_pos_2[];
layout(location = 3) in vec4 in_pos_3[];
layout(location = 4) in vec4 in_pos_4[];
layout(location = 5) in vec4 in_pos_5[];
layout(location = 6) in vec4 in_pos_6[];
layout(location = 7) in vec4 in_pos_7[];

layout(binding = 0, set = 0) uniform global_ubo { GlobalUbo ubo; };

void main() {
    const mat4 vpMat = ubo.projMat * ubo.viewMat;
    const vec4 p0 = vpMat * in_pos_0[0];
    const vec4 p1 = vpMat * in_pos_1[0];
    const vec4 p2 = vpMat * in_pos_2[0];
    const vec4 p3 = vpMat * in_pos_3[0];

    const vec4 p4 = vpMat * in_pos_4[0];
    const vec4 p5 = vpMat * in_pos_5[0];
    const vec4 p6 = vpMat * in_pos_6[0];
    const vec4 p7 = vpMat * in_pos_7[0];

    
    gl_Position = p0;
    EmitVertex();
    gl_Position = p1;
    EmitVertex();
    gl_Position = p3;
    EmitVertex();
    gl_Position = p2;
    EmitVertex();
    gl_Position = p0;
    EmitVertex();
    
    gl_Position = p4;
    EmitVertex();
    gl_Position = p5;
    EmitVertex();
    gl_Position = p7;
    EmitVertex();
    gl_Position = p6;
    EmitVertex();
    gl_Position = p4;
    EmitVertex();
    
    gl_Position = p5;
    EmitVertex();
    gl_Position = p1;
    EmitVertex();
    gl_Position = p3;
    EmitVertex();
    gl_Position = p7;
    EmitVertex();
    gl_Position = p6;
    EmitVertex();
    gl_Position = p2;
    EmitVertex();

    EndPrimitive();
}