#version 460 
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable
#include "defines.h"
#include "../utils/glsl/rendering_utils.glsl"
#include "../utils/glsl/bsdf/bsdf.glsl"
#include "../utils/glsl/random.glsl"

#define CONV_TEXTURE_BINDING BINDLESS_IMAGE_DESC_BINDING
#define CONV_TEXTURE_SET BINDLESS_IMAGE_DESC_SET
#include "../convenience/glsl/texture_data.glsl"

#define CONV_MATERIAL_BUFFER_BINDING GLOBAL_DESC_MATERIAL_BUFFER_BINDING
#define CONV_MATERIAL_BUFFER_SET GLOBAL_DESC_SET
#include "../convenience/glsl/material_data.glsl"

#define CONV_LIGHT_BUFFER_BINDING GLOBAL_DESC_LIGHT_BUFFER_BINDING
#define CONV_LIGHT_BUFFER_SET GLOBAL_DESC_SET
#include "../convenience/glsl/light_data.glsl"

layout(push_constant) uniform PushConstant {
    layout(offset = 0) mat4 modelMat;
    layout(offset = 64) int material_idx;
};

layout(binding = GLOBAL_DESC_UBO_BINDING, set = GLOBAL_DESC_SET) uniform global_ubo { GlobalUbo ubo; };

layout(location = 0) in vec4 v_color0;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_tangent;
layout(location = 3) in vec2 v_uv0;
layout(location = 4) in vec2 v_uv1;

noperspective layout(location = 9) in vec3 v_dist;
layout(location = 10) in vec3 frag_pos;

layout(location = 0) out vec4 fragColor;

vec3 punctualLights(in vec4 albedo, in vec3 ws_pos, in vec3 ws_nmap_normal, in vec3 ws_normal, const float roughness, const float metallic){
    vec3 result = vec3(0);
	
	

	for(int i = 0; i < ubo.light_count; i++){
		vec3 lightDir;
        
        
        if(isDirectionalLight(i)) lightDir = -1 * light_buffer[i].n_ws_norm.xyz;
        else lightDir = light_buffer[i].pos_ws.xyz - ws_pos;

        vec3 nN = normalize(ws_nmap_normal);
        const vec3 lightDirN = normalize(lightDir);
        const vec3 viewDirN = normalize(ubo.viewPos.xyz - ws_pos);
        if(dot(normalize(ws_normal), viewDirN) < 0) nN = -nN;

        const float dist = length(lightDir);
		const float NdotV = dot(nN, viewDirN);

        float intensity = light_buffer[i].intensity;
        float attenuation = 1.0f;
        float angularAttenuation = 1.0f;
        if(!isDirectionalLight(i)) attenuation = ATTENUATION_RANGE(dist, light_buffer[i].range);
        
        if(isSpotLight(i)){
            float cd = dot(light_buffer[i].n_ws_norm.xyz, -1 * lightDirN);
            angularAttenuation = clamp(cd * light_buffer[i].light_angle_scale + light_buffer[i].light_angle_offset,0.0,1.0);
            angularAttenuation *= angularAttenuation;
        }
        
        if(isIesLight(i)){ 
            intensity *= evalIesLightBilinear(i, -lightDirN);
        }

        BsdfScatterEvent e = { viewDirN.xyz, eLobeAll, ws_normal.xyz, ws_nmap_normal.xyz, metallic, albedo.xyz, roughness, 0, 1.0f, 1.5f, false };
        const vec3 brdf = metalroughness_eval(e, lightDirN);
        
		const vec3 shading = brdf * WATT_TO_RADIANT_INTENSITY(intensity) * light_buffer[i].color.xyz * attenuation * angularAttenuation;
		if(any(isnan(shading))) continue;
        result += shading;
	}
	return result;
}



float calcWireframeRatio() {
    const float d = min(abs(v_dist.x), min(abs(v_dist.y), abs(v_dist.z)));
 	return exp2(-2*d*d);
}

void main() {

    Material_s material = material_buffer[material_idx];
    
    vec4 baseColor = material.baseColorFactor;
    
    if(material.baseColorTexIdx != -1) baseColor *= texture(texture_sampler[material.baseColorTexIdx], v_uv0);
    if (material.alphaDiscard && baseColor.a <= material.alphaDiscardValue) discard;

    
    float occlusion = 0.0f;
    if(material.occlusionTexIdx != -1){
        occlusion = texture(texture_sampler[material.occlusionTexIdx], v_uv0).r;
        occlusion *= material.occlusionStrength;
        baseColor.xyz *= occlusion;
    }

    
	vec3 b = normalize(cross(v_normal, v_tangent));
    
    vec3 n_nmap_ws = v_normal;
    vec3 n_nmap_ts = vec3(0);
    if(material.normalTexIdx != -1){
        n_nmap_ts = texture(texture_sampler[material.normalTexIdx], v_uv0).xyz;
        n_nmap_ws = nTangentSpaceToWorldSpace(n_nmap_ts, material.normalScale, v_tangent, b, v_normal);
    }

    
    float metallic = 1.0f;
    if(material.metallicTexIdx != -1){
        metallic = texture(texture_sampler[material.metallicTexIdx], v_uv0).r;
    }
    metallic *= material.metallicFactor;
    
    float roughness = 1.0f;
    if(material.roughnessTexIdx != -1){
        roughness = texture(texture_sampler[material.roughnessTexIdx], v_uv0).r;
    }
    roughness *= material.roughnessFactor;
    
    
    roughness *= roughness;
    
    
    const bool useUV1 = material.lightTexCoordIdx == 1;
    vec3 lightMap = vec3(1.0f);
    if(material.lightTexIdx != -1){
        lightMap = texture(texture_sampler[material.lightTexIdx], useUV1 ? v_uv1 : v_uv0).xyz;
    }
    lightMap *= material.lightFactor;

    vec4 outColor = vec4(0,0,0,1);
    
    if(ubo.shade) {
        outColor.xyz += punctualLights((baseColor), frag_pos, n_nmap_ws, v_normal, roughness, metallic);
    } else {
        outColor.xyz = baseColor.xyz;
    }
    
    if(ubo.useLightMaps) {
        outColor.xyz *= lightMap.xyz;
    }

    
    if(ubo.display_mode == DISPLAY_MODE_VERTEX_COLOR && ubo.rgb_or_alpha == 0) outColor = vec4(v_color0.xyz,1);
    else if(ubo.display_mode == DISPLAY_MODE_VERTEX_COLOR && ubo.rgb_or_alpha == 1) outColor = vec4(v_color0.w,v_color0.w,v_color0.w,1);
    else if(ubo.display_mode == DISPLAY_MODE_METALLIC) outColor = vec4(vec3(metallic),1);
    else if(ubo.display_mode == DISPLAY_MODE_ROUGHNESS) outColor = vec4(vec3(roughness),1);
    else if(ubo.display_mode == DISPLAY_MODE_NORMAL_MAP) outColor = vec4(normalize(n_nmap_ts),1);
    else if(ubo.display_mode == DISPLAY_MODE_OCCLUSION) outColor = vec4(vec3(occlusion),1);
    else if(ubo.display_mode == DISPLAY_MODE_LIGHT_MAP) outColor = vec4(lightMap,1);

    if(ubo.mulVertexColors) outColor *= v_color0;

    
    if(ubo.wireframe){
        const float wr = calcWireframeRatio();
 	    outColor.xyz = wr * vec3(0.0) + (1.0 - wr) * outColor.xyz;
    }

    vec3 out_srgb = linearToSRGB(outColor.xyz);
    out_srgb = dither(out_srgb, ubo.dither_strength, randomFloatTriDist(gl_FragCoord.xy/ubo.size));
	fragColor = vec4(out_srgb, baseColor.w);
}