#version 460

layout(constant_id = 0) const uint ENTRIES_PER_VERTEX = 3;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

layout(push_constant) uniform PushConstant {
    layout(offset = 0) mat4 model_matrix;
};

layout(location = 0) out vec4 out_ws_pos;
layout(location = 1) out vec3 out_ws_normal;
layout(location = 2) out vec3 out_ws_tangent;
layout(location = 3) out uint out_radiance_buffer_offset;

void main() {
    out_ws_pos = model_matrix * vec4(position.xyz, 1.0);
    const mat3 normal_mat = mat3(transpose(inverse(model_matrix)));
    out_ws_normal = normalize(normal_mat * normal);
    out_ws_tangent = normalize(normal_mat * tangent);

    out_radiance_buffer_offset = gl_VertexIndex;
}