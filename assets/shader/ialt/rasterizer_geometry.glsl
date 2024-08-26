#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec4 in_color0[];
layout(location = 1) in vec3 in_normal[];
layout(location = 2) in vec3 in_tangent[];
layout(location = 3) in vec2 in_uv0[];
layout(location = 4) in uint in_radiance_buffer_offset[];
layout(location = 5) in uint in_radiance_weights_buffer_offset[];
layout(location = 8) in vec3 inPosWS[];

layout(binding = RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict uniform global_ubo{ GlobalBufferR ubo; };

layout(location = 0) out vec4 out_color0;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec2 out_uv0;
flat layout(location = 4) out uvec3 out_radiance_buffer_offsets;
flat layout(location = 5) out uvec3 out_radiance_weights_buffer_offsets;
layout(location = 8) out vec3 outPosWS;
layout(location = 9) out vec3 out_bary_coords;

noperspective layout(location = 10) out vec3 out_dist;

void main() {
    const vec2 p0 = ubo.size * gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    const vec2 p1 = ubo.size * gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    const vec2 p2 = ubo.size * gl_in[2].gl_Position.xy / gl_in[2].gl_Position.w;
    const vec2 v0 = p2 - p1;
    const vec2 v1 = p2 - p0;
    const vec2 v2 = p1 - p0;
    const float area = abs(v1.x * v2.y - v1.y * v2.x);

    const uvec3 radBuffOff = uvec3(in_radiance_buffer_offset[0], in_radiance_buffer_offset[1], in_radiance_buffer_offset[2]);
    const uvec3 radWBuffOff = uvec3(in_radiance_weights_buffer_offset[0], in_radiance_weights_buffer_offset[1], in_radiance_weights_buffer_offset[2]);

    gl_Position = gl_in[0].gl_Position;
    out_color0 = in_color0[0];
    out_normal = in_normal[0];
    out_tangent = in_tangent[0];
    out_uv0 = in_uv0[0];
    outPosWS = inPosWS[0];
    out_radiance_buffer_offsets = radBuffOff;
    out_radiance_weights_buffer_offsets = radWBuffOff;
    out_dist = vec3(area / length(v0), 0, 0);
    out_bary_coords = vec3(1,0,0);
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    out_color0 = in_color0[1];
    out_normal = in_normal[1];
    out_tangent = in_tangent[1];
    out_uv0 = in_uv0[1];
    outPosWS = inPosWS[1];
    out_radiance_buffer_offsets = radBuffOff;
    out_radiance_weights_buffer_offsets = radWBuffOff;
    out_dist = vec3(0, area / length(v1), 0);
    out_bary_coords = vec3(0,1,0);
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    out_color0 = in_color0[2];
    out_normal = in_normal[2];
    out_tangent = in_tangent[2];
    out_uv0 = in_uv0[2];
    outPosWS = inPosWS[2];
    out_radiance_buffer_offsets = radBuffOff;
    out_radiance_weights_buffer_offsets = radWBuffOff;
    out_dist = vec3(0, 0, area / length(v2));
    out_bary_coords = vec3(0,0,1);
    EmitVertex();

    EndPrimitive();
}
