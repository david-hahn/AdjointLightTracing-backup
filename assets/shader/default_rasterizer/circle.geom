#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

layout (points) in;
layout (triangle_strip, max_vertices = 6) out;

layout(location = 0) in vec4 in_ws_pos[];

layout(push_constant) uniform PushConstant {
    layout(offset = 28) float radius;
};

layout(binding = 0, set = 0) uniform global_ubo { GlobalUbo ubo; };

layout(location = 0) out vec2 out_uv;

void main() {
    const vec4 right = vec4(ubo.viewMat[0][0], ubo.viewMat[1][0], ubo.viewMat[2][0], 0);

    const float size = 1.1f;
    const mat4 vpMat = ubo.projMat * ubo.viewMat;
    const vec4 center = vpMat * in_ws_pos[0];
    const vec4 border = vpMat * (in_ws_pos[0] + radius * right);
    const float half_width = size*length(center.xyz - border.xyz);	
   
    const float ratio = ubo.size.x / ubo.size.y;

    const vec4 a = center + vec4(half_width, half_width * ratio, 0, 0);
    const vec4 b = center + vec4(half_width, -half_width * ratio, 0, 0);
    const vec4 c = center + vec4(-half_width, half_width * ratio, 0, 0);
    const vec4 d = center - vec4(half_width, half_width * ratio, 0, 0);

    gl_Position = a;
    out_uv = vec2(size);
    EmitVertex();
    gl_Position = b;
    out_uv = vec2(size,-size);
    EmitVertex();
    gl_Position = c;
    out_uv = vec2(-size,size);
    EmitVertex();
    EndPrimitive();

    gl_Position = c;
    out_uv = vec2(-size,size);
    EmitVertex();
    gl_Position = b;
    out_uv = vec2(size,-size);
    EmitVertex();
    gl_Position = d;
    out_uv = vec2(-size);
    EmitVertex();
    EndPrimitive();
}