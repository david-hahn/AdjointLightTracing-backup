#version 460
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout(push_constant) uniform PushConstant {
    layout(offset = 0) mat4 vp_mat;
};

layout(location = 0) out vec4 v_color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = vp_mat * vec4(position.xyz, 1.0);
    v_color = color;
}