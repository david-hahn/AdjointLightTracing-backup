#version 460

layout(push_constant) uniform PushConstant {
    layout(offset = 0) mat4 m;
    layout(offset = 64) vec3 bb_min;
    layout(offset = 80) vec3 bb_max;
};

layout(location = 0) out vec4 o_pos_0;
layout(location = 1) out vec4 o_pos_1;
layout(location = 2) out vec4 o_pos_2;
layout(location = 3) out vec4 o_pos_3;
layout(location = 4) out vec4 o_pos_4;
layout(location = 5) out vec4 o_pos_5;
layout(location = 6) out vec4 o_pos_6;
layout(location = 7) out vec4 o_pos_7;

void main() {
    o_pos_0 = m * vec4(bb_min.x, bb_min.y, bb_min.z, 1);
    o_pos_1 = m * vec4(bb_min.x, bb_min.y, bb_max.z, 1);
    o_pos_2 = m * vec4(bb_min.x, bb_max.y, bb_min.z, 1);
    o_pos_3 = m * vec4(bb_min.x, bb_max.y, bb_max.z, 1);

    o_pos_4 = m * vec4(bb_max.x, bb_min.y, bb_min.z, 1);
    o_pos_5 = m * vec4(bb_max.x, bb_min.y, bb_max.z, 1);
    o_pos_6 = m * vec4(bb_max.x, bb_max.y, bb_min.z, 1);
    o_pos_7 = m * vec4(bb_max.x, bb_max.y, bb_max.z, 1);
}