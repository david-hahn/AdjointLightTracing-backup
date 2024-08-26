#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "defines.h"
#include "../utils/glsl/rendering_utils.glsl"
#include "../utils/glsl/colormap.glsl"


layout(constant_id = 0) const uint SPHERICAL_HARMONIC_ORDER = 1;
layout(constant_id = 1) const uint ENTRIES_PER_VERTEX = 3;
layout(constant_id = 2) const uint USE_UNPHYSICAL_NICE_PREVIEW = 0;

#define CONV_TEXTURE_BINDING BINDLESS_IMAGE_DESC_BINDING
#define CONV_TEXTURE_SET BINDLESS_IMAGE_DESC_SET
#include "../convenience/glsl/texture_data.glsl"

#define CONV_MATERIAL_BUFFER_BINDING RASTERIZER_DESC_MATERIAL_BUFFER_BINDING
#define CONV_MATERIAL_BUFFER_SET RASTERIZER_DESC_SET
#include "../convenience/glsl/material_data.glsl"





layout(push_constant) uniform PushConstant{
    layout(offset = 0) mat4 modelMat;
    layout(offset = 64) int material_idx;
};

layout(location = 0) in vec4 in_color0;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv0;
flat layout(location = 4) in uvec3 in_rad_buff_off;
flat layout(location = 5) in uvec3 in_rad_w_buff_off;
layout(location = 8) in vec3 inPosWS;
layout(location = 9) in vec3 in_bary;
noperspective layout(location = 10) in vec3 in_dist;

layout(binding = RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict uniform global_ubo{ GlobalBufferR ubo; };
layout(binding = RASTERIZER_DESC_RADIANCE_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer radiance_storage_buffer { float radiance_buffer[]; };
layout(binding = RASTERIZER_DESC_TARGET_RADIANCE_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer target_radiance_storage_buffer { float target_radiance_buffer[]; };
layout(binding = RASTERIZER_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer target_radiance_weight_storage_buffer { float target_radiance_weights_buffer[]; };
layout(binding = RASTERIZER_DESC_CHANNEL_WEIGHTS_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer channel_weights_storage_buffer { float channel_weights_buffer[]; };
layout(binding = RASTERIZER_DESC_VTX_COLOR_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer vtx_color_storage_buffer { float vtx_color_buffer[]; };
layout(binding = RASTERIZER_DESC_AREA_BUFFER_BINDING, set = RASTERIZER_DESC_SET) readonly restrict buffer vertex_area_buffer { float vtxArea[]; };

#define SH_READ
#define SH_DATA_BUFFER_ radiance_buffer
#include "../sphericalharmonics/sphericalharmonics.glsl"
#define TARGET_SH_DATA_BUFFER_ target_radiance_buffer
#define CHANNEL_WEIGHTS_DATA_BUFFER_ channel_weights_buffer
#include "target_sh.glsl"

layout(location = 0) out vec4 fragColor;



float calcWireframeRatio() {
    const float d = min(abs(in_dist.x), min(abs(in_dist.y), abs(in_dist.z)));
    return exp2(-2 * d * d);
}

float doBary(const float v0, const float v1, const float v2) {
    return in_bary.x * v0 + in_bary.y * v1 + in_bary.z * v2;
}

void main() {
    Material_s material = material_buffer[material_idx];
    bool doSRGB = true;
    vec3 viewDir = normalize(ubo.viewPos.xyz - inPosWS);
    vec3 texColor = material.baseColorFactor.xyz;
    if (material.baseColorTexIdx != -1) texColor *= texture(texture_sampler[material.baseColorTexIdx], in_uv0).xyz;

    vec3 rgb = vec3(0.0);
    if (ubo.show_target) {
        if (ubo.show_alpha) {
            rgb = vec3(doBary(target_radiance_weights_buffer[in_rad_w_buff_off.x+0], target_radiance_weights_buffer[in_rad_w_buff_off.y+0], target_radiance_weights_buffer[in_rad_w_buff_off.z+0]));
        } else {
#ifndef USE_SPHERICAL_HARMONICS
            rgb[0] = doBary(target_radiance_buffer[in_rad_buff_off.x+0], target_radiance_buffer[in_rad_buff_off.y+0], target_radiance_buffer[in_rad_buff_off.z+0]);
            rgb[1] = doBary(target_radiance_buffer[in_rad_buff_off.x+1], target_radiance_buffer[in_rad_buff_off.y+1], target_radiance_buffer[in_rad_buff_off.z+1]);
            rgb[2] = doBary(target_radiance_buffer[in_rad_buff_off.x+2], target_radiance_buffer[in_rad_buff_off.y+2], target_radiance_buffer[in_rad_buff_off.z+2]);
#else 
            
#if USE_HSH
            rgb = vec3(evalInterHSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), in_rad_buff_off, in_bary, viewDir, in_normal, in_tangent));
#else 
            rgb = vec3(evalInterSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), in_rad_buff_off, in_bary, viewDir));
#endif 
#endif 
            
        } 
    } else if (ubo.show_adjoint_deriv) {
        const float weight = doBary(target_radiance_weights_buffer[in_rad_w_buff_off.x], 
            target_radiance_weights_buffer[in_rad_w_buff_off.y], 
            target_radiance_weights_buffer[in_rad_w_buff_off.z]);
#ifndef USE_SPHERICAL_HARMONICS
        const vec3 x = vec3(doBary(radiance_buffer[in_rad_buff_off.x+0], radiance_buffer[in_rad_buff_off.y+0], radiance_buffer[in_rad_buff_off.z+0]),
            doBary(radiance_buffer[in_rad_buff_off.x+1], radiance_buffer[in_rad_buff_off.y+1], radiance_buffer[in_rad_buff_off.z+1]),
            doBary(radiance_buffer[in_rad_buff_off.x+2], radiance_buffer[in_rad_buff_off.y+2], radiance_buffer[in_rad_buff_off.z+2]));
        const vec3 target = vec3(doBary(target_radiance_buffer[in_rad_buff_off.x+0], target_radiance_buffer[in_rad_buff_off.y+0], target_radiance_buffer[in_rad_buff_off.z+0]),
            doBary(target_radiance_buffer[in_rad_buff_off.x+1], target_radiance_buffer[in_rad_buff_off.y+1], target_radiance_buffer[in_rad_buff_off.z+1]),
            doBary(target_radiance_buffer[in_rad_buff_off.x+2], target_radiance_buffer[in_rad_buff_off.y+2], target_radiance_buffer[in_rad_buff_off.z+2]));
#else 

#if USE_HSH
        const vec3 x = vec3(evalInterHSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), in_rad_buff_off, in_bary, viewDir, in_normal, in_tangent));
        const vec3 target = vec3(evalInterHSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), in_rad_buff_off, in_bary, viewDir, in_normal, in_tangent));
#else 
        const vec3 x = 0.5f * vec3(evalInterSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), in_rad_buff_off, in_bary, viewDir));
        const vec3 target = 0.5f * vec3(evalInterSHSumRGBTarget(int(SPHERICAL_HARMONIC_ORDER), in_rad_buff_off, in_bary, viewDir));
#endif 
#endif 
        const vec3 xMinusTarget = (x - target);
        const float adjoint = xMinusTarget.x + xMinusTarget.y + xMinusTarget.z;
        
        const float t = ubo.log_adjoint_vis ? log(abs(adjoint) + 1.0f) / log(ubo.adjoint_range + 1.0f) : abs(adjoint) / ubo.adjoint_range;
        const float t2 = (sign(adjoint) * (max(0,min(t,1)))) + 1.0f;
        rgb.xyz = mix(vec3(1.0f), viridis_color_map(1.0f-t2/2.0f).xyz, vec3(weight));
        
        doSRGB = false;
    } else {
#ifndef USE_SPHERICAL_HARMONICS
        rgb[0] = doBary(radiance_buffer[in_rad_buff_off.x+0], radiance_buffer[in_rad_buff_off.y+0], radiance_buffer[in_rad_buff_off.z+0]);
        rgb[1] = doBary(radiance_buffer[in_rad_buff_off.x+1], radiance_buffer[in_rad_buff_off.y+1], radiance_buffer[in_rad_buff_off.z+1]);
        rgb[2] = doBary(radiance_buffer[in_rad_buff_off.x+2], radiance_buffer[in_rad_buff_off.y+2], radiance_buffer[in_rad_buff_off.z+2]);
#else 

#if USE_HSH
        vec3 normal = in_normal;
        vec3 tangent = in_tangent;

        
        rgb = in_color0.rgb; 

        
        
        
#else 
        rgb = vec3(evalInterSHSumRGB(int(SPHERICAL_HARMONIC_ORDER), in_rad_buff_off, in_bary, viewDir));
#endif 
#endif 
        
        if( USE_UNPHYSICAL_NICE_PREVIEW == 1 ){
            rgb *= texColor; 
        }
    }

    vec3 out_srgb = doSRGB ? linearToSRGB(rgb) : rgb;
    fragColor = vec4(out_srgb, 1);

    
    if (ubo.show_wireframe_overlay) {
        const float wr = calcWireframeRatio();
        fragColor.xyz = wr * ubo.wireframeColor.xyz + (1.0 - wr) * fragColor.xyz;
    }

    
    if( USE_UNPHYSICAL_NICE_PREVIEW == 1 && material.isEmissive ){
        fragColor = vec4(material.emissionFactor, 1);
        if (material.emissionTexIdx != -1) fragColor.xyz *= texture(texture_sampler[material.emissionTexIdx], in_uv0).xyz;
    }
}
