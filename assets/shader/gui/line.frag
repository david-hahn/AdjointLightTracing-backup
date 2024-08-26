#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec4 v_color;

void main() {
    fragColor = v_color;

}