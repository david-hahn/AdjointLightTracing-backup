#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

layout (points) in;
layout (triangle_strip, max_vertices = 36) out;

layout(location = 0) in vec4 in_ws_pos[];
layout(location = 1) in vec3 in_ws_normal[];
layout(location = 2) in vec3 in_ws_tangent[];
layout(location = 3) in uint in_radiance_buffer_offset[];

layout(push_constant) uniform PushConstant {
    layout(offset = 64) float radius;
};

layout(binding = RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, set = 0) readonly restrict uniform global_ubo { GlobalBufferR global_buffer; };

layout(location = 0) out vec3 position_ws;
layout(location = 1) out vec3 normal_ws;
layout(location = 2) out vec3 tangent_ws;
flat layout(location = 3) out vec3 center_ws;
flat layout(location = 4) out uint out_radiance_buffer_offset;

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

    const bool c_v0 = all(lessThanEqual(abs(v0_cs.xy), vec2(v0_cs.w))) && 0.0f < v0_cs.z && v0_cs.z < v0_cs.w;
    const bool c_v1 = all(lessThanEqual(abs(v1_cs.xy), vec2(v1_cs.w))) && 0.0f < v1_cs.z && v1_cs.z < v1_cs.w;
    const bool c_v2 = all(lessThanEqual(abs(v2_cs.xy), vec2(v2_cs.w))) && 0.0f < v2_cs.z && v2_cs.z < v2_cs.w;
    const bool c_v3 = all(lessThanEqual(abs(v3_cs.xy), vec2(v3_cs.w))) && 0.0f < v3_cs.z && v3_cs.z < v3_cs.w;
    const bool c_v4 = all(lessThanEqual(abs(v4_cs.xy), vec2(v4_cs.w))) && 0.0f < v4_cs.z && v4_cs.z < v4_cs.w;
    const bool c_v5 = all(lessThanEqual(abs(v5_cs.xy), vec2(v5_cs.w))) && 0.0f < v5_cs.z && v5_cs.z < v5_cs.w;
    const bool c_v6 = all(lessThanEqual(abs(v6_cs.xy), vec2(v6_cs.w))) && 0.0f < v6_cs.z && v6_cs.z < v6_cs.w;
    const bool c_v7 = all(lessThanEqual(abs(v7_cs.xy), vec2(v7_cs.w))) && 0.0f < v7_cs.z && v7_cs.z < v7_cs.w;

    
    if(c_v0 || c_v2 || c_v3) {
        gl_Position = v0_cs;
        position_ws = v0.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v2_cs;
        position_ws = v2.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v3_cs;
        position_ws = v3.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    if(c_v0 || c_v3 || c_v1) {
        gl_Position = v0_cs;
        position_ws = v0.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v3_cs;
        position_ws = v3.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v1_cs;
        position_ws = v1.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    
    if(c_v2 || c_v6 || c_v7) {
        gl_Position = v2_cs;
        position_ws = v2.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v6_cs;
        position_ws = v6.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v7_cs;
        position_ws = v7.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    if(c_v2 || c_v7 || c_v3) {
        gl_Position = v2_cs;
        position_ws = v2.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v7_cs;
        position_ws = v7.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v3_cs;
        position_ws = v3.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    
    if(c_v6 || c_v4 || c_v5) {
        gl_Position = v6_cs;
        position_ws = v6.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v4_cs;
        position_ws = v4.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v5_cs;
        position_ws = v5.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    if(c_v6 || c_v5 || c_v7) {
        gl_Position = v6_cs;
        position_ws = v6.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v5_cs;
        position_ws = v5.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v7_cs;
        position_ws = v7.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    
    if(c_v4 || c_v0 || c_v1) {
        gl_Position = v4_cs;
        position_ws = v4.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v0_cs;
        position_ws = v0.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v1_cs;
        position_ws = v1.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    if(c_v4 || c_v1 || c_v5) {
        gl_Position = v4_cs;
        position_ws = v4.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v1_cs;
        position_ws = v1.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v5_cs;
        position_ws = v5.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    
    if(c_v0 || c_v4 || c_v6) {
        gl_Position = v0_cs;
        position_ws = v0.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v4_cs;
        position_ws = v4.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v6_cs;
        position_ws = v6.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    if(c_v0 || c_v6 || c_v2) {
        gl_Position = v0_cs;
        position_ws = v0.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v6_cs;
        position_ws = v6.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v2_cs;
        position_ws = v2.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    
    if(c_v1 || c_v5 || c_v7) {
        gl_Position = v1_cs;
        position_ws = v1.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v5_cs;
        position_ws = v5.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v7_cs;
        position_ws = v7.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }

    if(c_v1 || c_v7 || c_v3) {
        gl_Position = v1_cs;
        position_ws = v1.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v7_cs;
        position_ws = v7.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        gl_Position = v3_cs;
        position_ws = v3.xyz;
        normal_ws = in_ws_normal[0];
        tangent_ws = in_ws_tangent[0];
        center_ws = in_ws_pos[0].xyz;
        out_radiance_buffer_offset = in_radiance_buffer_offset[0];
        EmitVertex();
        EndPrimitive();
    }
}