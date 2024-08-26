#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

layout(push_constant) uniform PushConstant {
    layout(offset = 0) mat4 m;
    layout(offset = 64) float scale;
};

layout(location = 0) out vec4 p_center;
layout(location = 1) out vec4 p_normal;
layout(location = 2) out vec4 p_tangent;
layout(location = 3) out vec4 p_bitangent;

void main() {
    const vec3 b = normalize(cross(normal, tangent));
    p_center = m * vec4(position, 1);
    p_normal = p_center + vec4(normalize(mat3(transpose(inverse(m))) * normal.xyz) * scale,0);
    p_tangent = p_center + vec4(normalize(mat3(transpose(inverse(m))) * tangent.xyz) * scale,0);
    p_bitangent = p_center + vec4(normalize(mat3(transpose(inverse(m))) * b.xyz) * scale,0);
}