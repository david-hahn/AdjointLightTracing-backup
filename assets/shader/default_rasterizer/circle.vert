#version 460

layout(push_constant) uniform PushConstant {
    layout(offset = 0) vec4 ws_pos;
};

layout(location = 0) out vec4 out_ws_pos;

void main() {
    out_ws_pos = vec4(ws_pos.xyz, 1);
}