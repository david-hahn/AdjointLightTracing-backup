#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec4 in_color0[];
layout(location = 1) in vec3 in_normal[];
layout(location = 2) in vec3 in_tangent[];
layout(location = 3) in vec2 in_uv0[];
layout(location = 4) in vec2 in_uv1[];
layout(location = 10) in vec3 in_pos[];

layout(binding = GLOBAL_DESC_UBO_BINDING, set = GLOBAL_DESC_SET) uniform global_ubo { GlobalUbo ubo; };

layout(location = 0) out vec4 out_color0;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec2 out_uv0;
layout(location = 4) out vec2 out_uv1;

noperspective layout(location = 9) out vec3 out_dist;
layout(location = 10) out vec3 out_pos;

void main() {
    const vec2 p0 = ubo.size * gl_in[0].gl_Position.xy/gl_in[0].gl_Position.w;
    const vec2 p1 = ubo.size * gl_in[1].gl_Position.xy/gl_in[1].gl_Position.w;
    const vec2 p2 = ubo.size * gl_in[2].gl_Position.xy/gl_in[2].gl_Position.w;
    const vec2 v0 = p2-p1;
    const vec2 v1 = p2-p0;
    const vec2 v2 = p1-p0;
    const float area = abs(v1.x*v2.y - v1.y * v2.x);

    gl_Position = gl_in[0].gl_Position; 
    out_color0 = in_color0[0];
    out_normal = in_normal[0];
    out_tangent = in_tangent[0];
    out_uv0 = in_uv0[0];
    out_uv1 = in_uv1[0];
    out_pos = in_pos[0];
    out_dist = vec3(area/length(v0),0,0);
    EmitVertex();

    gl_Position = gl_in[1].gl_Position; 
    out_color0 = in_color0[1];
    out_normal = in_normal[1];
    out_tangent = in_tangent[1];
    out_uv0 = in_uv0[1];
    out_uv1 = in_uv1[1];
    out_pos = in_pos[1];
    out_dist = vec3(0,area/length(v1),0);
    EmitVertex();

    gl_Position = gl_in[2].gl_Position; 
    out_color0 = in_color0[2];
    out_normal = in_normal[2];
    out_tangent = in_tangent[2];
    out_uv0 = in_uv0[2];
    out_uv1 = in_uv1[2];
    out_pos = in_pos[2];
    out_dist = vec3(0,0,area/length(v2));
    EmitVertex();
   
    EndPrimitive();
}