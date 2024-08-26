#version 460

layout(constant_id = 0) const uint ENTRIES_PER_VERTEX = 3;

layout(push_constant) uniform PushConstant {
    layout(offset = 0) vec4 position_ws;
};
layout(location = 0) out vec4 out_ws_pos;

void main() {
    out_ws_pos = vec4(position_ws.xyz, 1.0);
}
