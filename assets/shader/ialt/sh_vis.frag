#version 460 
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable
#include "defines.h"
#include "../utils/glsl/rendering_utils.glsl"
#include "../utils/glsl/colormap.glsl"

layout(constant_id = 0) const uint SPHERICAL_HARMONIC_ORDER = 1;
layout(constant_id = 1) const uint ENTRIES_PER_VERTEX = 3;

layout(location = 0) in vec3 position_ws;
layout(location = 1) in vec3 normal_ws;
layout(location = 2) in vec3 tangent_ws;
flat layout(location = 3) in vec3 center_ws;
flat layout(location = 4) in uint radiance_buffer_offset;

layout(push_constant) uniform PushConstant {
    layout(offset = 64) float radius;
};

layout(binding = RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, set = 0) readonly restrict uniform global_ubo { GlobalBufferR ubo; };
layout(binding = RASTERIZER_DESC_RADIANCE_BUFFER_BINDING, set = 0) readonly restrict buffer radiance_storage_buffer { float radiance_buffer[]; };
layout(binding = RASTERIZER_DESC_TARGET_RADIANCE_BUFFER_BINDING, set = 0) readonly restrict buffer target_radiance_storage_buffer { float target_radiance_buffer[]; };
layout(binding = RASTERIZER_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, set = 0) readonly restrict buffer target_radiance_weight_storage_buffer { float target_radiance_weights_buffer[]; };

#define SH_READ
#define SH_DATA_BUFFER_ radiance_buffer
#include "../sphericalharmonics/sphericalharmonics.glsl"
#define TARGET_SH_DATA_BUFFER_ target_radiance_buffer
#include "target_sh.glsl"

layout(location = 0) out vec4 fragColor;

bool sphereIntersect(const vec3 ro, const vec3 rd, const vec4 sph, out float t)
{
    const vec3 oc = ro - sph.xyz;
    const float b = dot(oc, rd);
    const float c = dot(oc, oc) - sph.w * sph.w;
    float h = b * b - c;
    if(h < 0.0f) return false;
    h = sqrt(h);
    t = -b - h;
    return true;
}

void main() {
    const vec3 viewDir = normalize(position_ws.xyz - ubo.viewPos.xyz);

    float t = 0.0f;
    const bool hit = sphereIntersect(ubo.viewPos.xyz, viewDir, vec4(center_ws.xyz, radius), t);
    vec3 hitPos = ubo.viewPos.xyz + viewDir * t; 

    const vec3 shDir = normalize(hitPos - center_ws.xyz);
    vec3 sh = vec3(0.0f);
#if USE_HSH
    if(dot(shDir, normal_ws) < 0) discard;
    if (ubo.show_target) {
        sh = vec3(evalHSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir, normal_ws, tangent_ws));
    } else if (ubo.show_adjoint_deriv) {
        const vec3 x = vec3(evalHSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir, normal_ws, tangent_ws));
        const vec3 target = vec3(evalHSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir, normal_ws, tangent_ws));
        const vec3 xMinusTarget = (x - target);
        const float adjoint = xMinusTarget.x + xMinusTarget.y + xMinusTarget.z;
        
        const float t = ubo.log_adjoint_vis ? log(abs(adjoint) + 1.0f) / log(ubo.adjoint_range + 1.0f) : abs(adjoint) / ubo.adjoint_range;
        const float t2 = (sign(adjoint) * (max(0,min(t,1)))) + 1.0f;
        sh.xyz = viridis_color_map(1.0f-t2/2.0f).xyz;
    } else {
        sh = vec3(evalHSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir, normal_ws, tangent_ws));
    }
#else 
    if (ubo.show_target) {
        sh = 0.5f * vec3(evalSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir));
    } else if (ubo.show_adjoint_deriv) {

        const vec3 x = 0.5f * vec3(evalSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir));
        const vec3 target = 0.5f * vec3(evalSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir));
        const vec3 xMinusTarget = (x - target);
        const float adjoint = xMinusTarget.x + xMinusTarget.y + xMinusTarget.z;
        
        const float t = ubo.log_adjoint_vis ? log(abs(adjoint) + 1.0f) / log(ubo.adjoint_range + 1.0f) : abs(adjoint) / ubo.adjoint_range;
        const float t2 = (sign(adjoint) * (max(0,min(t,1)))) + 1.0f;
        sh.xyz = viridis_color_map(1.0f-t2/2.0f).xyz;
    } else {
        sh = 0.5f * vec3(evalSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), int(radiance_buffer_offset * ENTRIES_PER_VERTEX), shDir));
    }
#endif 
    

    if(hit) fragColor = vec4(linearToSRGB(sh.xyz), 1);
    else discard;

    const mat4 vpMat = ubo.projMat * ubo.viewMat;
    const vec4 v_cs = vpMat * vec4(hitPos.xyz, 1);
    gl_FragDepth = 1.0f - (v_cs.z / v_cs.w);
}