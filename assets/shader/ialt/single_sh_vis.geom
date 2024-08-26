#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

layout (points) in;
layout (triangle_strip, max_vertices = 36) out;

layout(location = 0) in vec4 in_ws_pos[];

layout(push_constant) uniform PushConstant {
    layout(offset = 40) float radius;
};

layout(binding = RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, set = 0) readonly restrict uniform global_ubo { GlobalBufferR global_buffer; };

layout(location = 0) out vec3 position_ws;
flat layout(location = 1) out vec3 center_ws;

void main() {
    const mat4 vpMat = global_buffer.projMat * global_buffer.viewMat;
    const float half_width = radius;

    
    const vec4 v0 = in_ws_pos[0] + vec4(-half_width, half_width, half_width, 0);
    const vec4 v1 = in_ws_pos[0] + vec4(-half_width, -half_width, half_width, 0);
    const vec4 v2 = in_ws_pos[0] + vec4(half_width, half_width, half_width, 0);
    const vec4 v3 = in_ws_pos[0] + vec4(half_width, -half_width, half_width, 0);
    const vec4 v4 = in_ws_pos[0] + vec4(-half_width, half_width, -half_width, 0);
    const vec4 v5 = in_ws_pos[0] + vec4(-half_width, -half_width, -half_width, 0);
    const vec4 v6 = in_ws_pos[0] + vec4(half_width, half_width, -half_width, 0);
    const vec4 v7 = in_ws_pos[0] + vec4(half_width, -half_width, -half_width, 0);

    const vec4 v0_cs = vpMat * v0;
    const vec4 v1_cs = vpMat * v1;
    const vec4 v2_cs = vpMat * v2;
    const vec4 v3_cs = vpMat * v3;
    const vec4 v4_cs = vpMat * v4;
    const vec4 v5_cs = vpMat * v5;
    const vec4 v6_cs = vpMat * v6;
    const vec4 v7_cs = vpMat * v7;

    
    gl_Position = v0_cs;
    position_ws = v0.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v2_cs;
    position_ws = v2.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v3_cs;
    position_ws = v3.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    gl_Position = v0_cs;
    position_ws = v0.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v3_cs;
    position_ws = v3.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v1_cs;
    position_ws = v1.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    
    gl_Position = v2_cs;
    position_ws = v2.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v6_cs;
    position_ws = v6.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v7_cs;
    position_ws = v7.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    gl_Position = v2_cs;
    position_ws = v2.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v7_cs;
    position_ws = v7.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v3_cs;
    position_ws = v3.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    
    gl_Position = v6_cs;
    position_ws = v6.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v4_cs;
    position_ws = v4.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v5_cs;
    position_ws = v5.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    gl_Position = v6_cs;
    position_ws = v6.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v5_cs;
    position_ws = v5.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v7_cs;
    position_ws = v7.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    
    gl_Position = v4_cs;
    position_ws = v4.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v0_cs;
    position_ws = v0.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v1_cs;
    position_ws = v1.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    gl_Position = v4_cs;
    position_ws = v4.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v1_cs;
    position_ws = v1.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v5_cs;
    position_ws = v5.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    
    gl_Position = v0_cs;
    position_ws = v0.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v4_cs;
    position_ws = v4.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v6_cs;
    position_ws = v6.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    gl_Position = v0_cs;
    position_ws = v0.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v6_cs;
    position_ws = v6.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v2_cs;
    position_ws = v2.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    
    gl_Position = v1_cs;
    position_ws = v1.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v5_cs;
    position_ws = v5.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v7_cs;
    position_ws = v7.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();

    gl_Position = v1_cs;
    position_ws = v1.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v7_cs;
    position_ws = v7.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    gl_Position = v3_cs;
    position_ws = v3.xyz;
    center_ws = in_ws_pos[0].xyz;
    EmitVertex();
    EndPrimitive();
}
