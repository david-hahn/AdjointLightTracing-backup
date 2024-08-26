#version 460
#extension GL_GOOGLE_include_directive : require
#include "defines.h"

#define CONV_MATERIAL_BUFFER_BINDING GLOBAL_DESC_MATERIAL_BUFFER_BINDING
#define CONV_MATERIAL_BUFFER_SET GLOBAL_DESC_SET
#include "../convenience/glsl/material_data.glsl"

layout(push_constant) uniform PushConstant {
    layout(offset = 0) mat4 modelMat;
    layout(offset = 64) int material_idx;
};
layout(binding = GLOBAL_DESC_UBO_BINDING, set = GLOBAL_DESC_SET) uniform global_ubo { GlobalUbo ubo; };

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 uv0;
layout(location = 4) in vec2 uv1;
layout(location = 5) in vec4 color0;

layout(location = 0) out vec4 v_color0;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_tangent;
layout(location = 3) out vec2 v_uv0;
layout(location = 4) out vec2 v_uv1;

layout(location = 10) out vec3 frag_pos;

void main() {
    gl_Position = ubo.projMat * ubo.viewMat * modelMat * vec4(position.xyz, 1.0);

    frag_pos = vec3(modelMat * vec4(position.xyz, 1.0));

    v_color0 = color0;
    v_uv0 = uv0;
    v_uv1 = uv1;
    v_normal = normalize(mat3(transpose(inverse(modelMat))) * normal.xyz);
    v_tangent = normalize(mat3(transpose(inverse(modelMat))) * tangent.xyz);
}