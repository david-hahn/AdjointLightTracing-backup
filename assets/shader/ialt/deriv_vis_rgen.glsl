#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_atomic_float : require
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_nonuniform_qualifier : require

#define GLSL
#define BXDFS_EVAL_NO_COS
#include "defines.h"
#include "../utils/glsl/ray_tracing_utils.glsl"
#include "../utils/glsl/rendering_utils.glsl"
#include "../utils/glsl/colormap.glsl"
#include "../utils/glsl/random.glsl"
#include "../utils/glsl/bsdf/bsdf.glsl"

layout(constant_id = 0) const uint SPHERICAL_HARMONIC_ORDER = 1;
layout(constant_id = 1) const uint ENTRIES_PER_VERTEX = 3;


#define CONV_TEXTURE_BINDING BINDLESS_IMAGE_DESC_BINDING
#define CONV_TEXTURE_SET BINDLESS_IMAGE_DESC_SET
#include "../convenience/glsl/texture_data.glsl"

#define CONV_MATERIAL_BUFFER_BINDING ADJOINT_DESC_MATERIAL_BUFFER_BINDING
#define CONV_MATERIAL_BUFFER_SET ADJOINT_DESC_SET
#include "../convenience/glsl/material_data.glsl"

#define CONV_INDEX_BUFFER_BINDING ADJOINT_DESC_INDEX_BUFFER_BINDING
#define CONV_INDEX_BUFFER_SET ADJOINT_DESC_SET
#define CONV_VERTEX_BUFFER_BINDING ADJOINT_DESC_VERTEX_BUFFER_BINDING
#define CONV_VERTEX_BUFFER_SET ADJOINT_DESC_SET
#include "../convenience/glsl/vertex_data.glsl"

#define CONV_TLAS_BINDING ADJOINT_DESC_TLAS_BINDING
#define CONV_TLAS_SET ADJOINT_DESC_SET
#define CONV_GEOMETRY_BUFFER_BINDING ADJOINT_DESC_GEOMETRY_BUFFER_BINDING
#define CONV_GEOMETRY_BUFFER_SET ADJOINT_DESC_SET
#include "../convenience/glsl/as_data.glsl"

#define CONV_LIGHT_BUFFER_BINDING ADJOINT_DESC_LIGHT_BUFFER_BINDING
#define CONV_LIGHT_BUFFER_SET ADJOINT_DESC_SET
#include "../convenience/glsl/light_data.glsl"

#include "../utils/glsl/bsdf/bsdf.glsl"
#include "../utils/glsl/random.glsl"
#include "functions.glsl"

layout(binding = ADJOINT_DESC_INFO_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer info_storage_buffer { AdjointInfo_s info; };
layout(binding = ADJOINT_DESC_RADIANCE_BUFFER_BINDING, set = ADJOINT_DESC_SET) buffer radiance_storage_buffer { float radiance_buffer[]; };
layout(binding = ADJOINT_DESC_AREA_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer vertex_area_buffer { float vtxArea[]; };

layout(binding = DERIV_VIS_DESC_DERIV_IMAGE_BINDING, set = DERIV_VIS_DESC_SET, rgba8) uniform image2D output_image;
layout(binding = DERIV_VIS_DESC_DERIV_ACC_IMAGE_BINDING, set = DERIV_VIS_DESC_SET, rgba32f) uniform image2D output_image_acc;
layout(binding = DERIV_VIS_DESC_DERIV_ACC_COUNT_IMAGE_BINDING, set = DERIV_VIS_DESC_SET, r32ui) uniform uimage2D output_image_acc_count;

layout(binding = DERIV_VIS_DESC_UBO_BUFFER_BINDING, set = DERIV_VIS_DESC_SET) readonly restrict uniform global_ubo{ GlobalBufferR ubo; };
layout(binding = DERIV_VIS_DESC_TARGET_BUFFER_BINDING, set = DERIV_VIS_DESC_SET) readonly restrict buffer target_radiance_storage_buffer { float target_radiance_buffer[]; };
layout(binding = DERIV_VIS_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING, set = DERIV_VIS_DESC_SET) readonly restrict buffer target_radiance_weight_storage_buffer { float target_radiance_weights_buffer[]; };
layout(binding = DERIV_VIS_DESC_FD_RADIANCE_BUFFER_BINDING, set = DERIV_VIS_DESC_SET) readonly restrict buffer fd_radiance_storage_buffer { float fd_radiance_buffer[]; };
layout(binding = DERIV_VIS_DESC_FD2_RADIANCE_BUFFER_BINDING, set = DERIV_VIS_DESC_SET) readonly restrict buffer fd2_radiance_storage_buffer { float fd2_radiance_buffer[]; };



layout(location = 0) rayPayloadEXT AdjointPayload ap;

#define SH_DATA_BUFFER_ radiance
#include "../sphericalharmonics/sphericalharmonics.glsl"

uint seed;

float doBary(const vec3 bary, const float v0, const float v1, const float v2) {
    return bary.x * v0 + bary.y * v1 + bary.z * v2;
}

double doBary(const dvec3 bary, const double v0, const double v1, const double v2) {
    return bary.x * v0 + bary.y * v1 + bary.z * v2;
}


vec2 getScreenCoordinates(const vec2 sample_radius){
	
	vec2 pSampleOffset = (vec2(tea_nextFloat(seed), tea_nextFloat(seed)) * 2.0f * sample_radius) - sample_radius;
	vec2 pSamplePos = vec2(gl_LaunchIDEXT.xy) + vec2(0.5f); 	
	pSamplePos += pSampleOffset;							    
	const vec2 inUV = pSamplePos/vec2(gl_LaunchSizeEXT.xy);
	const vec2 screenCoordClipSpace  = inUV * 2.0f - 1.0f;
	return screenCoordClipSpace;
}
RayDesc getRay(const vec2 sccs)
{
	const vec4 origin = ubo.inverseViewMat * vec4(0,0,0,1);
	const vec4 target = ubo.inverseProjMat * vec4(sccs.x, sccs.y, 1, 1) ;
	const vec4 direction = ubo.inverseViewMat * vec4(normalize(target.xyz), 0) ;
	RayDesc ray;
	ray.origin = origin.xyz;
	ray.direction = normalize(direction.xyz);
	ray.tmin = 1e-6f;
    ray.tmax = 1e+6f;
	return ray;
}

void storeColorMap(float value, float value2) {
    
    vec2 accParamGrad = imageLoad(output_image_acc, ivec2(gl_LaunchIDEXT.xy)).xy;
    uint count = imageLoad(output_image_acc_count, ivec2(gl_LaunchIDEXT.xy)).x;
    if (ubo.grad_vis_accumulate) {
        count++;
        value = (accParamGrad.x + value);
        value2 = (accParamGrad.x + value2);
        imageStore(output_image_acc, ivec2(gl_LaunchIDEXT.xy), vec4(value, value2, 0.0f, 0.0f));
        imageStore(output_image_acc_count, ivec2(gl_LaunchIDEXT.xy), uvec4(count));
    } else {
        imageStore(output_image_acc, ivec2(gl_LaunchIDEXT.xy), vec4(value, value2, 0.0f, 0.0f));
        imageStore(output_image_acc_count, ivec2(gl_LaunchIDEXT.xy), uvec4(1u));
    }
    value /= float(count);
    value2 /= float(count);

    
    
    
    
    
    
    const float t = ubo.log_grad_vis ? log(abs(value) + 1.0f) / log(ubo.grad_range + 1.0f) : abs(value) / ubo.grad_range;
    
    
    
    const float t2 = sign(value) * (max(0,min(t,1))) + 1.0f;
    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), vec4(coolwarm_color_map(t2/2.0f), 1.0f));
}

void finiteDiff(const bool visible, in HitDataStruct hd) {

    dvec3 target = dvec3(0);
    dvec3 fhp = dvec3(0);
    dvec3 fhn = dvec3(0);
    dvec3 current = dvec3(0);
    const vec3 dir = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), hd.hitN));
    for (int l = 0; l <= SPHERICAL_HARMONIC_ORDER; l++) {
        const float smoothing = 1.0f / (1.0f + SH_SMOOTHING * float((l * (l + 1)) * (l * (l + 1))));
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint shIdx = getSHindex(l, m);
#if USE_HSH
            float shWeight = evalHSH(l, m, dir, hd.hitN, hd.hitT);
#else 
            float shWeight = evalSH(l, m, dir);
#endif 
            
            [[unroll, dependency_infinite]] 
            for (uint i = 0u; i < 3u; i++) { 
                
                target[i] += doBary(dvec3(hd.bary),
                    double(target_radiance_weights_buffer[hd.idx[0]] * target_radiance_buffer[hd.radBuffIdx.x + shIdx * 3 + i]),
                    double(target_radiance_weights_buffer[hd.idx[1]] * target_radiance_buffer[hd.radBuffIdx.y + shIdx * 3 + i]),
                    double(target_radiance_weights_buffer[hd.idx[2]] * target_radiance_buffer[hd.radBuffIdx.z + shIdx * 3 + i])) * double(shWeight);
                current[i] += doBary(dvec3(hd.bary),
                    double(target_radiance_weights_buffer[hd.idx[0]] * radiance_buffer[hd.radBuffIdx.x + shIdx * 3 + i] ),
                    double(target_radiance_weights_buffer[hd.idx[1]] * radiance_buffer[hd.radBuffIdx.y + shIdx * 3 + i] ),
                    double(target_radiance_weights_buffer[hd.idx[2]] * radiance_buffer[hd.radBuffIdx.z + shIdx * 3 + i] )) * double(shWeight);
                

                fhp[i] += doBary(dvec3(hd.bary),
                    double(fd_radiance_buffer[hd.radBuffIdx.x + shIdx * 3 + i]),
                    double(fd_radiance_buffer[hd.radBuffIdx.y + shIdx * 3 + i]),
                    double(fd_radiance_buffer[hd.radBuffIdx.z + shIdx * 3 + i])) * double(shWeight);
                fhn[i] += doBary(dvec3(hd.bary),
                    double(fd2_radiance_buffer[hd.radBuffIdx.x + shIdx * 3 + i]),
                    double(fd2_radiance_buffer[hd.radBuffIdx.y + shIdx * 3 + i]),
                    double(fd2_radiance_buffer[hd.radBuffIdx.z + shIdx * 3 + i])) * double(shWeight);
            }
        }
    }

    
    const dvec3 imageGradient = (current - target);
    
    const dvec3 lightGradient = (fhp - fhn) / (2.0 * double(ubo.fd_grad_h));

    
    double cd = 0.0;
if(GRAD_VIS_PARTIAL == GV_COMPLETE) {
    cd = dot(imageGradient, lightGradient);
} else if(GRAD_VIS_PARTIAL == GV_LIGHT_GRAD) {
    cd = lightGradient.x + lightGradient.y + lightGradient.z;
} else if(GRAD_VIS_PARTIAL == GV_IMAGE_GRAD) {
    cd = imageGradient.x + imageGradient.y + imageGradient.z;
}

    storeColorMap(visible ? float(cd) : 0.0f, 0.0f);
}

const float tinyEps = 1e-5;

void geometricDerivativeRect(
    inout vec3 dcodp, inout vec3 dcodn, inout vec3 dcodt, inout vec3 dcir2dp, inout vec3 dcir2dn, inout vec3 dcir2dt,
    const vec3 lp, const vec3 ld, const vec3 lt, const vec3 hp, const vec3 n,
    const float dx, const float dy
) 
{
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float ln1 = ld.x; const float ln2 = ld.y; const float ln3 = ld.z;
    const float lt1 = lt.x; const float lt2 = lt.y; const float lt3 = lt.z;
    const float hx1 = hp.x; const float hx2 = hp.y; const float hx3 = hp.z;
    const float hn1 = n.x; const float  hn2 = n.y; const float  hn3 = n.z;
    #include "codegen/_codegen_rectLightDerivs_DH.h"
    if(any(isnan(dcodp))) dcodp = vec3(0.0f);
    if(any(isnan(dcodn))) dcodn = vec3(0.0f);
    if(any(isnan(dcodt))) dcodt = vec3(0.0f);
    if(any(isnan(dcir2dp))) dcir2dp = vec3(0.0f);
    if(any(isnan(dcir2dn))) dcir2dn = vec3(0.0f);
    if(any(isnan(dcir2dt))) dcir2dt = vec3(0.0f);
}

vec2 relativeOffsetAlongExtents;



vec3 evalLight(const uint index, const in HitDataStruct hd, inout vec3 f_l_dir_norm, inout vec3 lightPos){
    if( index >= info.light_count ) return vec3(0.0);
	Light_s light = light_buffer[index];

	vec3 f_l_dir;
	float f_l_dist;
    float lightWeight = 1.0;

    lightPos = light.pos_ws.xyz;

    if( isPointLight(index) || isSpotLight(index) || isRectangleLight(index) || isSquareLight(index)|| isIesLight(index)){
        if (isRectangleLight(index) || isSquareLight(index)) {
            relativeOffsetAlongExtents = tea_nextFloat2(seed) - 0.5f;
            const vec3 light_b_ws_norm = normalize(cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz));

            lightPos = light.pos_ws.xyz + relativeOffsetAlongExtents.x * light.dimensions.x * light.t_ws_norm.xyz + relativeOffsetAlongExtents.y * light.dimensions.y * light_b_ws_norm;
        }

		f_l_dir = lightPos - hd.hitX;
		f_l_dir_norm = normalize(f_l_dir);
		f_l_dist = length(f_l_dir);

		float attenuation = dot(hd.texNormal, f_l_dir_norm) / (f_l_dist * f_l_dist);
        float angularAttenuation = 1.0f;

		
		if( isSpotLight(index) ){
            const float cosTheta = dot(light.n_ws_norm.xyz, -f_l_dir_norm);
            if( cosTheta < cos(light.inner_angle) && cosTheta > cos(light.outer_angle) ){
                const float light_angle_scale = 1.0 / (cos(light.inner_angle) - cos(light.outer_angle));
                const float light_angle_offset = -cos(light.outer_angle) * light_angle_scale;
                angularAttenuation = cosTheta * light.light_angle_scale + light.light_angle_offset;
		        angularAttenuation *= angularAttenuation;
            }
            else if( cosTheta < cos(light.outer_angle) ) angularAttenuation = 0.0;
		} else if (isRectangleLight(index) || isSquareLight(index)) {
            const float cosTheta = dot(light.n_ws_norm.xyz, -f_l_dir_norm);
            if(cosTheta <= tinyEps) return vec3(0.0);
            attenuation *= 2*cosTheta;
        } else if(isIesLight(index)) {
            attenuation *= evalIesLightBilinear(index, -f_l_dir_norm);
        }

		lightWeight *= WATT_TO_RADIANT_INTENSITY(light.intensity) * attenuation * angularAttenuation;
	}
	
	if( dot(hd.hitN,f_l_dir_norm)  <= tinyEps){ return vec3(0.0); } 
    if(isRectangleLight(index) || isSquareLight(index)) lightWeight *= 0.5f;

    traceRayEXT(tlas, gl_RayFlagsTerminateOnFirstHitEXT, 0xff, 0, 0, 0, hd.hitX, tinyEps, f_l_dir_norm, f_l_dist - tinyEps, 0);
    if (ap.instanceIndex != -1) return vec3(0.0); 
	return light.color * lightWeight;
}


#ifdef PT_VERSION

vec3 evaluateBRDF(const in vec3 wi, const in vec3 wo, const in HitDataStruct hd){
    const BsdfScatterEvent e = { wi, eLobeAll, hd.texNormal, hd.texNormal, hd.texMetallic, hd.texColor, hd.texRoughness, hd.texTransmission, hd.eta_ext, hd.eta_int, false };

    #if USE_GGX
        return metalroughness_eval(e, wo);
    #else
        return lambert_brdf_eval(e, wo); 
    #endif
}

void fetchTriangleData(const uint geoIdx, const uint primIdx, const in vec3 bary, out HitDataStruct hd) {
    hd.bary = bary;
    const GeometryData_s geometry_data = geometry_buffer[geoIdx];
    const Material_s material = material_buffer[geometry_data.data_index];

    
    hd.idx = uvec3(primIdx * 3) + uvec3(0, 1, 2);
    if ((geometry_data.flags & eGeoDataIndexedTriangleBit) > 0)
    {
        hd.idx[0] = index_buffer[geometry_data.index_buffer_offset + hd.idx[0]];
        hd.idx[1] = index_buffer[geometry_data.index_buffer_offset + hd.idx[1]];
        hd.idx[2] = index_buffer[geometry_data.index_buffer_offset + hd.idx[2]];
    }
    hd.idx += geometry_data.vertex_buffer_offset;
    
    hd.radBuffIdx = hd.idx * ENTRIES_PER_VERTEX;

    
    const mat3 normal_mat = mat3(transpose(inverse(geometry_data.model_matrix)));
    hd.n0 = normalize( normal_mat * vertex_buffer[hd.idx[0]].normal.xyz );
    hd.n1 = normalize( normal_mat * vertex_buffer[hd.idx[1]].normal.xyz );
    hd.n2 = normalize( normal_mat * vertex_buffer[hd.idx[2]].normal.xyz );
    hd.hitN = normalize((hd.n0 * hd.bary[0] + hd.n1 * hd.bary[1] + hd.n2 * hd.bary[2]));
    
    hd.t0 = normalize( normal_mat * vertex_buffer[hd.idx[0]].tangent.xyz );
    hd.t1 = normalize( normal_mat * vertex_buffer[hd.idx[1]].tangent.xyz );
    hd.t2 = normalize( normal_mat * vertex_buffer[hd.idx[2]].tangent.xyz );
    hd.hitT = normalize((hd.t0 * hd.bary[0] + hd.t1 * hd.bary[1] + hd.t2 * hd.bary[2])); 
    
    hd.p0 = (geometry_data.model_matrix * vertex_buffer[hd.idx[0]].position).xyz;
    hd.p1 = (geometry_data.model_matrix * vertex_buffer[hd.idx[1]].position).xyz;
    hd.p2 = (geometry_data.model_matrix * vertex_buffer[hd.idx[2]].position).xyz;
    hd.hitX = hd.bary[0] * hd.p0 + hd.bary[1] * hd.p1 + hd.bary[2] * hd.p2;

    if ( abs(dot(hd.hitN, hd.hitN)-1.0) > 1e-5 ) hd.hitN = normalize( hd.hitN );
    if ( abs(dot(hd.hitT, hd.hitT)-1.0) > 1e-5 || abs(dot(hd.hitN, hd.hitT)) > 1e-5) hd.hitT = normalize( getPerpendicularVector(hd.hitN) ); 

    
    hd.texColor = material.baseColorFactor.xyz;
    hd.texNormal = hd.hitN; 
    hd.texMetallic = material.metallicFactor; hd.texRoughness = material.roughnessFactor;
    hd.texTransmission = material.transmissionFactor; 
    hd.eta_int = material.ior; hd.eta_ext = 1.0f;
    {
        const vec2 uv_0 = vertex_buffer[hd.idx[0]].texture_coordinates_0.xy;
        const vec2 uv_1 = vertex_buffer[hd.idx[1]].texture_coordinates_0.xy;
        const vec2 uv_2 = vertex_buffer[hd.idx[2]].texture_coordinates_0.xy;
        const vec2 uv = (hd.bary.x * uv_0 + hd.bary.y * uv_1 + hd.bary.z * uv_2);
        if (material.baseColorTexIdx != -1) {
            hd.texColor *= (texture(texture_sampler[material.baseColorTexIdx], uv)).xyz;
        }
        if (material.metallicTexIdx != -1) {
            hd.texMetallic *= texture(texture_sampler[material.metallicTexIdx], uv).r;
        }
        if (material.roughnessTexIdx != -1) {
            hd.texRoughness *= texture(texture_sampler[material.roughnessTexIdx], uv).r;
        }
        if (material.transmissionTexIdx != -1) {
            hd.texTransmission *= texture(texture_sampler[material.transmissionTexIdx], uv).r;
        }
        
        
        
        
    }
}

void symbolicBRDFderiv(out vec3 dbrdfdp[3], 
    const vec3 lp, const vec3 n, const vec3 wo, const vec3 albedo, 
    const float metallic, const float roughness, float eta_int, float eta_ext){
    const vec3 hp = vec3(0.0);
    const float alpha = roughness;
    const float alpha2 = alpha * alpha;
    const vec3 wi = normalize(lp - hp);
    const vec3 m = normalize(wi + wo);
    const float NdotWi = dot(n, wi);
    const float NdotWo = dot(n, wo);
    const float MdotWi = dot(m, wi);
    const float MdotWo = dot(m, wo);
    const float MdotN = dot(m, n);

    float cosT;
    const float fo = F_Dielectric(eta_ext, eta_int, abs(NdotWo), cosT);
    const float eta = eta_ext / eta_int;
	const float eta2 = eta * eta;

    const float re = fresnelDiffuseReflectance(eta_ext, eta_int);
	const float ri = 1.0f - eta2 * (1.0f - re);
	const vec3 diffuse = (1.0f - metallic) * eta2 * albedo * (1.0f - fo) / (M_PI * (1.0f - (PLASTIC_NONLINEAR ? albedo : vec3(1.0f)) * ri));

    const float n1 = n.x; const float n2 = n.y; const float n3 = n.z;
    const float wo1 = wo.x; const float wo2 = wo.y; const float wo3 = wo.z;
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float hp1 = hp.x; const float hp2 = hp.y; const float hp3 = hp.z;
    const float albedo1 = albedo.x; const float albedo2 = albedo.y; const float albedo3 = albedo.z;
    
    vec3 gradDiffuse = vec3(0.0f);
    {
        const float tinyEps = 0.0;
        #include "codegen/_codegen_diffuseBrdfDerivsWi.h"
    }
    vec3 gradSpecular = vec3(0.0f);
    {
        const float tinyEps = 0.0;
        #include "codegen/_codegen_specularBrdfDerivsWi.h"
    }

    if(any(isnan(gradDiffuse))) gradDiffuse = vec3(0.0f);
    if(any(isnan(gradSpecular)) || all(equal(wi + wo, vec3(0.0f))) || MdotN <= 0.0f || (MdotWo / NdotWo) <= 0 || (MdotWi / NdotWi) <= 0) gradSpecular = vec3(0.0f);
	if(NdotWi <= 0.0f || NdotWo <= 0.0f) { gradDiffuse = vec3(0.0f); gradSpecular = vec3(0.0f); }
    if((MdotWo / NdotWo) <= 0 || (MdotWi / NdotWi) <= 0 || MdotN <= 0 || all(equal(wi + wo, vec3(0.0f)))) gradSpecular = vec3(0.0f);
    
    dbrdfdp[0] = diffuse.x * gradDiffuse + gradSpecular;
    dbrdfdp[1] = diffuse.y * gradDiffuse + gradSpecular;
    dbrdfdp[2] = diffuse.z * gradDiffuse + gradSpecular;
}

void computeBrdfDerivative(out vec3 dBrdfdWiR, out vec3 dBrdfdWiG, out vec3 dBrdfdWiB, const in vec3 wi, const in vec3 wo, const in HitDataStruct hd){
    #if USE_GGX
        
        vec3 wi_fd; const float fd_h = 1e-2;
        wi_fd = wi + vec3(fd_h, 0.0, 0.0); wi_fd = normalize(wi_fd);
        vec3 brdf_x = evaluateBRDF( wi_fd, wo, hd);
        wi_fd = wi + vec3(0.0, fd_h, 0.0); wi_fd = normalize(wi_fd);
        vec3 brdf_y = evaluateBRDF( wi_fd, wo, hd);
        wi_fd = wi + vec3(0.0, 0.0, fd_h); wi_fd = normalize(wi_fd);
        vec3 brdf_z = evaluateBRDF( wi_fd, wo, hd);
        vec3 brdf_0 = evaluateBRDF( wi, wo, hd);
        vec3 dBrdfdWiR_fd = vec3( brdf_x.r-brdf_0.r , brdf_y.r-brdf_0.r , brdf_z.r-brdf_0.r ) / fd_h;
        vec3 dBrdfdWiG_fd = vec3( brdf_x.g-brdf_0.g , brdf_y.g-brdf_0.g , brdf_z.g-brdf_0.g ) / fd_h;
        vec3 dBrdfdWiB_fd = vec3( brdf_x.b-brdf_0.b , brdf_y.b-brdf_0.b , brdf_z.b-brdf_0.b ) / fd_h;

        
        vec3 dbrdfdp[3];
        symbolicBRDFderiv(dbrdfdp, wi, hd.texNormal, wo, hd.texColor, hd.texMetallic, hd.texRoughness, hd.eta_int, hd.eta_ext);
        dBrdfdWiR = dbrdfdp[0]; dBrdfdWiG = dbrdfdp[1]; dBrdfdWiB = dbrdfdp[2];

        
        

        if( any(isnan(dBrdfdWiR)) || any(isnan(dBrdfdWiG)) || any(isnan(dBrdfdWiB)) ){
            dBrdfdWiR = dBrdfdWiG = dBrdfdWiB = vec3(0.0);
            debugPrintfEXT("BRDF deriv NaN");
        }

    #else 
        dBrdfdWiR = dBrdfdWiG = dBrdfdWiB = vec3(0.0);
    #endif
}


void processLocalSample(const in HitDataStruct hd, const vec3 wo, const float sampleWeight, const vec3 incidentRadiance, 
    const vec3 incidentDirection, inout vec3 dOdRadiance, const bool firstHit, inout vec3 dOdwi){
    if( vtxArea[hd.idx[0]] < (tinyEps*tinyEps) || vtxArea[hd.idx[1]] < (tinyEps*tinyEps) || vtxArea[hd.idx[2]] < (tinyEps*tinyEps) ) return;
    if( dot(wo, hd.hitN) < tinyEps ) return;

    float objFcnPartial[3][ENTRIES_PER_VERTEX];
    for(uint idx = 0u; idx < ENTRIES_PER_VERTEX; idx++) {
        objFcnPartial[0][idx] = target_radiance_weights_buffer[hd.idx[0]] * (radiance_buffer[hd.radBuffIdx[0] + idx] - target_radiance_buffer[hd.radBuffIdx[0] + idx]) ;
        objFcnPartial[1][idx] = target_radiance_weights_buffer[hd.idx[1]] * (radiance_buffer[hd.radBuffIdx[1] + idx] - target_radiance_buffer[hd.radBuffIdx[1] + idx]) ;
        objFcnPartial[2][idx] = target_radiance_weights_buffer[hd.idx[2]] * (radiance_buffer[hd.radBuffIdx[2] + idx] - target_radiance_buffer[hd.radBuffIdx[2] + idx]) ;
        
    }

    const vec3 brdf = evaluateBRDF( incidentDirection, wo, hd);

    vec3 dFlux = sampleWeight * brdf;

    vec3 dBRDF = sampleWeight * incidentRadiance;
    vec3 dBrdfdWiR = vec3(0.0), dBrdfdWiG = vec3(0.0), dBrdfdWiB = vec3(0.0);
    if( firstHit ){ computeBrdfDerivative(dBrdfdWiR, dBrdfdWiG, dBrdfdWiB, incidentDirection, wo, hd); }


    
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= SPHERICAL_HARMONIC_ORDER; l++) {
        const float smoothing = 1.0f / (1.0f + SH_SMOOTHING * float((l * (l + 1)) * (l * (l + 1)))); 
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint shIdx = getSHindex(l, m);

            for(uint k = 0u; k < 3u; ++k){ 
                vec3 n_k, t_k;
                if      (k==0) { n_k=hd.n0; t_k=hd.t0; }
                else if (k==1) { n_k=hd.n1; t_k=hd.t1; }
                else if (k==2) { n_k=hd.n2; t_k=hd.t2; }

                
                if( abs(dot(n_k, n_k) - 1.0) > tinyEps ){
                    
                    n_k = normalize(n_k);
                }
                if( abs(dot(t_k, t_k) - 1.0) > tinyEps || abs(dot(n_k, t_k)) > tinyEps || any(isnan(t_k)) ){
                    
                    t_k = normalize(getPerpendicularVector(n_k)); 
                }

                float shWeight = 0.0f;
                if(dot(wo, n_k) > tinyEps) { shWeight = evalHSH(l, m, wo, n_k, t_k) * smoothing; }

                [[unroll, dependency_infinite]]
                for (uint i = 0u; i < 3u; i++) { 
                    if(GRAD_VIS_PARTIAL == GV_COMPLETE) {
                        dOdRadiance[i] += dFlux[i] * objFcnPartial[k][shIdx * 3 + i] * shWeight * hd.bary[k];
                    } else if(GRAD_VIS_PARTIAL == GV_LIGHT_GRAD) {
                        dOdRadiance[i] += dFlux[i] * shWeight * hd.bary[k];
                    } else if(GRAD_VIS_PARTIAL == GV_IMAGE_GRAD) {}
                }
                if( firstHit ){

                    if(GRAD_VIS_PARTIAL == GV_COMPLETE) {
                        dOdwi += dBrdfdWiR * dBRDF[0] * objFcnPartial[k][shIdx * 3 + 0] * shWeight * hd.bary[k];
                        dOdwi += dBrdfdWiG * dBRDF[1] * objFcnPartial[k][shIdx * 3 + 1] * shWeight * hd.bary[k];
                        dOdwi += dBrdfdWiB * dBRDF[2] * objFcnPartial[k][shIdx * 3 + 2] * shWeight * hd.bary[k];
                    } else if(GRAD_VIS_PARTIAL == GV_LIGHT_GRAD) {
                        dOdwi += dBrdfdWiR * dBRDF[0] * shWeight * hd.bary[k];
                        dOdwi += dBrdfdWiG * dBRDF[1] * shWeight * hd.bary[k];
                        dOdwi += dBrdfdWiB * dBRDF[2] * shWeight * hd.bary[k];
                    } else if(GRAD_VIS_PARTIAL == GV_IMAGE_GRAD) {}
                }
            }
        }
    }
}

uint sampleLight(){
    return ubo.light_deriv_idx;
}

void main()
{
    
    seed = tea_init(uint(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x), ubo.frameCount);
    
    vec3 outColorValue = vec3(ubo.bg_color);
    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), vec4(outColorValue, 1.0f));
    
    const vec2 sccs = getScreenCoordinates(vec2(0.5f));
	RayDesc rayDesc = getRay(sccs);

    BounceDataStruct bd = BounceDataStruct(vec3(1.0f), 1.0f, vec3(0.0f), 0.0f, 0u);

    LightGradDataRGB lgd = LightGradDataRGB(
        vec3[3](vec3(0.0f), vec3(0.0f), vec3(0.0f)),
        vec3[3](vec3(0.0f), vec3(0.0f), vec3(0.0f)),
        vec3[3](vec3(0.0f), vec3(0.0f), vec3(0.0f)),
        vec2[3](vec2(0.0f), vec2(0.0f), vec2(0.0f))
    );
    ObjGradData ogd = ObjGradData(vec3(0.0f));

    rayDesc.flags = gl_RayFlagsNoneEXT;
    if (ubo.cull_mode == 1) rayDesc.flags = gl_RayFlagsCullFrontFacingTrianglesEXT;
    else if(ubo.cull_mode == 2) rayDesc.flags = gl_RayFlagsCullBackFacingTrianglesEXT;
    
    traceRayEXT(tlas, rayDesc.flags, 0xff, 0, 0, 0, rayDesc.origin, rayDesc.tmin, rayDesc.direction, rayDesc.tmax, 0);
    if ( ap.instanceIndex == -1) return; 
    


    const uint samplePointsN = gl_LaunchSizeEXT.y;
    
    vec3 bary = computeBarycentric(ap.hitAttribute);

    HitDataStruct localSamplePoint;

    fetchTriangleData(ap.customInstanceID + ap.geometryIndex, ap.primitiveIndex, bary, localSamplePoint);
    float triArea = length(cross(localSamplePoint.p1 - localSamplePoint.p0, localSamplePoint.p2 - localSamplePoint.p0)) * 0.5f;

    
    const float sampleWeight = 1.0f / float(BOUNCE_SAMPLES);

    
    vec3 directIncidentDirection = vec3(0.0);
    uint lightIdx = sampleLight();

    vec3 lightPos;
    vec3 directIncidentRadiance = evalLight(lightIdx, localSamplePoint, directIncidentDirection, lightPos);

    
    vec3 hitToLight = lightPos - localSamplePoint.hitX;
    RayDesc shadowRay;
    shadowRay.origin = localSamplePoint.hitX; 
    shadowRay.direction = normalize(hitToLight);
    shadowRay.tmin = 0.01f;
    shadowRay.tmax = length(hitToLight) - 0.01f;
    traceRayEXT(tlas, gl_RayFlagsTerminateOnFirstHitEXT, 0xff, 0, 0, 0, shadowRay.origin, shadowRay.tmin, shadowRay.direction, shadowRay.tmax, 0);
    if (ap.instanceIndex != -1) return;
    if (ubo.fd_grad_vis) {
        finiteDiff(true, localSamplePoint);
        return;
    }
    
    vec3 dOdP = vec3(0);
    vec3 dOdN = vec3(0);
    vec3 dOdT = vec3(0);
    float dOdI = 0;
    vec3 dOdC = vec3(0);
    vec2 dOdA = vec2(0);

    

    if( length(directIncidentRadiance) > tinyEps ){
        vec3 dOdDirectRadiance = vec3(0.0);
        vec3 dOdwi = vec3(0.0);
        mat3 dwidp = mat3(0.0);
        ParamDerivsDataStruct dLightdp = ParamDerivsDataStruct(vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0.0f), 0.0f, vec3(0.0f));

        computeParametricDerivatives( dLightdp, dwidp, localSamplePoint, lightIdx );
        

        for(uint k=0; k < BOUNCE_SAMPLES; ++k ){
            
            vec3 wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), localSamplePoint.hitN));
            
            
            
            processLocalSample(localSamplePoint, wo, sampleWeight, directIncidentRadiance, directIncidentDirection, dOdDirectRadiance, true, dOdwi);

            if( any(isnan(dOdDirectRadiance)) ) debugPrintfEXT("dOdDirectRadiance is nan");
            if( any(isnan(dOdwi)) ) debugPrintfEXT("dOdwi is nan");
        }

        vec3 dOdp_brdf = dOdwi * dwidp;
        if( any(isnan(dOdp_brdf)) ) debugPrintfEXT("dOdp_brdf is nan");

        dOdP = dot(light_buffer[lightIdx].color, dOdDirectRadiance) * dLightdp.position + dOdp_brdf;
        dOdN = dot(light_buffer[lightIdx].color, dOdDirectRadiance) * dLightdp.normal;
        dOdT = dot(light_buffer[lightIdx].color, dOdDirectRadiance) * dLightdp.tangent;
        dOdI = dot(light_buffer[lightIdx].color, dOdDirectRadiance) * dLightdp.intensity;
        dOdC = dOdDirectRadiance * dLightdp.color;
        dOdA = dot(light_buffer[lightIdx].color, dOdDirectRadiance) * dLightdp.angles;
        
    }
    
    
    HitDataStruct hd = localSamplePoint;

    
    const vec3 indirectIncidentDirection = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), hd.hitN));
    
    float pdf = PDF_UNIT_HEMISPHERE_UNIFORM;


    vec3 rayDirection = indirectIncidentDirection;
    vec3 rayImportance = vec3( dot(indirectIncidentDirection, localSamplePoint.texNormal) / pdf );
    vec3 indirectIncidentRadiance = vec3( 0.0 );

    
    vec3 indirectLocalDirections[BOUNCE_SAMPLES];
    for(uint k=0; k < BOUNCE_SAMPLES; ++k ){
        
        const vec3 wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), localSamplePoint.hitN));
        

        indirectLocalDirections[k] = wo;
    }


    
    vec3 dOdIndirectRadiance = vec3(0.0);
    vec3 unused = vec3(0.0);

    for(uint k=0; k < BOUNCE_SAMPLES; ++k ){
        
        
        

        const vec3 wo = indirectLocalDirections[k];
        processLocalSample(localSamplePoint, wo, sampleWeight, vec3(1.0), indirectIncidentDirection, dOdIndirectRadiance, false, unused);
    }

    uint depth = 0; bool missed = false;
    while( depth < info.bounces && !missed ){
        ++depth;

        
        traceRayEXT(tlas, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, hd.hitX, 1e-6f, rayDirection,  1e+6f, 0);

        if( ap.instanceIndex == -1){ missed = true; }
        else{
            fetchTriangleData(ap.customInstanceID + ap.geometryIndex, ap.primitiveIndex, computeBarycentric(ap.hitAttribute), hd); 
            
            
            vec3 remoteIncidentDirection = vec3(0.0);
            uint remoteLightIdx = sampleLight();
            vec3 remoteIncidentRadiance = evalLight(remoteLightIdx, hd, remoteIncidentDirection);

            if( length(remoteIncidentRadiance) > tinyEps ){
                
                const vec3 remoteBRDF = evaluateBRDF( remoteIncidentDirection, -rayDirection, hd);

                
                
                mat3 dwidp = mat3(0.0);
                ParamDerivsDataStruct dLightdp = ParamDerivsDataStruct(vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0.0f), 0.0f, vec3(0.0f));
                computeParametricDerivatives( dLightdp, dwidp, hd, remoteLightIdx );

                vec3 dBrdfdWiR = vec3(0.0), dBrdfdWiG = vec3(0.0), dBrdfdWiB = vec3(0.0);
                computeBrdfDerivative(dBrdfdWiR, dBrdfdWiG, dBrdfdWiB, remoteIncidentDirection, -rayDirection, hd);
                
                vec3 dOdL = rayImportance * remoteIncidentRadiance * dOdIndirectRadiance;
                
                vec3 dOdp_brdf = (dBrdfdWiR + dBrdfdWiG + dBrdfdWiB ) * dwidp;

               

                const vec3 dOdFlux = rayImportance * remoteBRDF * dOdIndirectRadiance;
                dOdP += dot(light_buffer[lightIdx].color, dOdFlux) * dLightdp.position + dOdp_brdf;
                dOdN += dot(light_buffer[lightIdx].color, dOdFlux) * dLightdp.normal;
                dOdT += dot(light_buffer[lightIdx].color, dOdFlux) * dLightdp.tangent;
                dOdI += dot(light_buffer[lightIdx].color, dOdFlux) * dLightdp.intensity;
                dOdC += dOdFlux * dLightdp.color;
                dOdA += dot(light_buffer[lightIdx].color, dOdFlux) * dLightdp.angles;
            }

            
            
            
            const vec3 nextRayDirection = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereCosine(tea_nextFloat2(seed)), hd.hitN));
            
            pdf = PDF_UNIT_HEMISPHERE_COSINE( dot(nextRayDirection,hd.hitN ) ); 

            
            const vec3 indirectBRDF = evaluateBRDF(nextRayDirection, -rayDirection, hd);

            rayImportance *= indirectBRDF * dot(nextRayDirection, hd.texNormal) / pdf;
            rayDirection = nextRayDirection;
        }
    }
    

    
    
    float paramGrad = 0.0;
    switch(ubo.param_deriv_idx){
        case 0: paramGrad = dOdP.x; break; 
        case 1: paramGrad = dOdP.y; break; 
        case 2: paramGrad = dOdP.z; break; 
        case 3: paramGrad = dOdI; break; 
        
        
        
        
        
        
        
        
    }

    
    if (GRAD_VIS_PARTIAL == GV_IMAGE_GRAD) paramGrad = ogd.dOdPhi.x + ogd.dOdPhi.y + ogd.dOdPhi.z;
    if (isnan(paramGrad)) return;

    storeColorMap(paramGrad, 0.0f);
}


#else 



void fetchHitPointData(inout HitDataStruct hd){
    /* GATHER GEO DATA */
    hd.bary = computeBarycentric(ap.hitAttribute);
    const GeometryData_s geometry_data = geometry_buffer[ap.customInstanceID + ap.geometryIndex];
    const Material_s material = material_buffer[geometry_data.data_index];
  
    
    hd.idx = uvec3(ap.primitiveIndex * 3) + uvec3(0, 1, 2);
    if ((geometry_data.flags & eGeoDataIndexedTriangleBit) > 0)
    {
        hd.idx[0] = index_buffer[geometry_data.index_buffer_offset + hd.idx[0]];
        hd.idx[1] = index_buffer[geometry_data.index_buffer_offset + hd.idx[1]];
        hd.idx[2] = index_buffer[geometry_data.index_buffer_offset + hd.idx[2]];
    }
    hd.idx += geometry_data.vertex_buffer_offset;
    
    hd.radBuffIdx = hd.idx * ENTRIES_PER_VERTEX;

    
    const mat3 normal_mat = mat3(transpose(inverse(geometry_data.model_matrix)));
    hd.n0 = normalize( normal_mat * vertex_buffer[hd.idx[0]].normal.xyz );
    hd.n1 = normalize( normal_mat * vertex_buffer[hd.idx[1]].normal.xyz );
    hd.n2 = normalize( normal_mat * vertex_buffer[hd.idx[2]].normal.xyz );
    hd.hitN = normalize( (hd.n0 * hd.bary[0] + hd.n1 * hd.bary[1] + hd.n2 * hd.bary[2]));
    
    hd.t0 = normalize( normal_mat * vertex_buffer[hd.idx[0]].tangent.xyz );
    hd.t1 = normalize( normal_mat * vertex_buffer[hd.idx[1]].tangent.xyz );
    hd.t2 = normalize( normal_mat * vertex_buffer[hd.idx[2]].tangent.xyz );
    hd.hitT = normalize((hd.t0 * hd.bary[0] + hd.t1 * hd.bary[1] + hd.t2 * hd.bary[2])); 
    
    hd.v0 = (geometry_data.model_matrix * vertex_buffer[hd.idx[0]].position).xyz;
    hd.v1 = (geometry_data.model_matrix * vertex_buffer[hd.idx[1]].position).xyz;
    hd.v2 = (geometry_data.model_matrix * vertex_buffer[hd.idx[2]].position).xyz;
    hd.hitX = hd.bary[0] * hd.v0 + hd.bary[1] * hd.v1 + hd.bary[2] * hd.v2;

    
    if( abs(dot(hd.hitN, hd.hitN) - 1.0) > tinyEps ){
        
        hd.hitN = normalize(hd.hitN);
    }
    if( abs(dot(hd.hitT, hd.hitT) - 1.0) > tinyEps || abs(dot(hd.hitN, hd.hitT)) > tinyEps || any(isnan(hd.hitT)) ){
        
        hd.hitT = normalize(getPerpendicularVector(hd.hitN)); 
    }

    

    
    hd.texColor = material.baseColorFactor.xyz;
    hd.texNormal = hd.hitN;
    hd.texMetallic = material.metallicFactor;
    hd.texRoughness = material.roughnessFactor;
    hd.texTransmission = material.transmissionFactor;
    hd.eta_int = material.ior; 
    hd.eta_ext = 1.0f;
    {
        const vec2 uv_0 = vertex_buffer[hd.idx[0]].texture_coordinates_0.xy;
        const vec2 uv_1 = vertex_buffer[hd.idx[1]].texture_coordinates_0.xy;
        const vec2 uv_2 = vertex_buffer[hd.idx[2]].texture_coordinates_0.xy;
        const vec2 uv = (hd.bary.x * uv_0 + hd.bary.y * uv_1 + hd.bary.z * uv_2);
        if (material.baseColorTexIdx != -1) {
            hd.texColor *= (texture(texture_sampler[material.baseColorTexIdx], uv)).xyz;
        }
        if (material.metallicTexIdx != -1) {
            hd.texMetallic *= texture(texture_sampler[material.metallicTexIdx], uv).r;
        }
        if (material.roughnessTexIdx != -1) {
            hd.texRoughness *= texture(texture_sampler[material.roughnessTexIdx], uv).r;
        }
        if (material.transmissionTexIdx != -1) {
            hd.texTransmission *= texture(texture_sampler[material.transmissionTexIdx], uv).r;
        }
        
        
        
        
        
    }
}

void computeParametricDerivatives(inout ParamDerivsDataStruct dWeightdp, out mat3 dwidp, const in HitDataStruct hd, const in uint light_idx, bool visible){
    const Light_s light = light_buffer[light_idx];
    vec3 rayOrigin = light.pos_ws.xyz;
    if (isRectangleLight(light_idx) || isSquareLight(light_idx)) {
        const vec3 light_b_ws_norm = normalize(cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz));
        rayOrigin = light.pos_ws.xyz + relativeOffsetAlongExtents.x * light.dimensions.x * light.t_ws_norm.xyz + relativeOffsetAlongExtents.y * light.dimensions.y * light_b_ws_norm;
    }
    const float dist = length(hd.hitX - rayOrigin);

    vec3 rayDirection = normalize( hd.hitX - rayOrigin );

    
    const float p1 = rayOrigin.x; const float p2 = rayOrigin.y; const float p3 = rayOrigin.z;
    const float n1 = hd.hitN.x; const float n2 = hd.hitN.y; const float n3 = hd.hitN.z;
    const float x1 = hd.hitX.x; const float x2 = hd.hitX.y; const float x3 = hd.hitX.z;
    vec3 dCosOverR2dp;
    float cosOverR2 = (1.0/sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*(n1*p1+n2*p2+n3*p3-n1*x1-n2*x2-n3*x3))/(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0));
    dCosOverR2dp[0] = (n1*1.0/sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0)))/(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))-(p1*2.0-x1*2.0)*1.0/sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*1.0/pow(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),2.0)*(n1*p1+n2*p2+n3*p3-n1*x1-n2*x2-n3*x3)-((p1*2.0-x1*2.0)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0)*(n1*p1+n2*p2+n3*p3-n1*x1-n2*x2-n3*x3)*(1.0/2.0))/(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0));
    dCosOverR2dp[1] = (n2*1.0/sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0)))/(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))-(p2*2.0-x2*2.0)*1.0/sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*1.0/pow(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),2.0)*(n1*p1+n2*p2+n3*p3-n1*x1-n2*x2-n3*x3)-((p2*2.0-x2*2.0)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0)*(n1*p1+n2*p2+n3*p3-n1*x1-n2*x2-n3*x3)*(1.0/2.0))/(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0));
    dCosOverR2dp[2] = (n3*1.0/sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0)))/(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))-(p3*2.0-x3*2.0)*1.0/sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*1.0/pow(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),2.0)*(n1*p1+n2*p2+n3*p3-n1*x1-n2*x2-n3*x3)-((p3*2.0-x3*2.0)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0)*(n1*p1+n2*p2+n3*p3-n1*x1-n2*x2-n3*x3)*(1.0/2.0))/(tinyEps+pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0));

    
    dwidp[0][0] = 1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0)*(p2*x2*-2.0-p3*x3*2.0+p2*p2+p3*p3+x2*x2+x3*x3);
    dwidp[0][1] = -(p1-x1)*(p2-x2)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0);
    dwidp[0][2] = -(p1-x1)*(p3-x3)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0);
    dwidp[1][0] = -(p1-x1)*(p2-x2)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0);
    dwidp[1][1] = 1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0)*(p1*x1*-2.0-p3*x3*2.0+p1*p1+p3*p3+x1*x1+x3*x3);
    dwidp[1][2] = -(p2-x2)*(p3-x3)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0);
    dwidp[2][0] = -(p1-x1)*(p3-x3)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0);
    dwidp[2][1] = -(p2-x2)*(p3-x3)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0);
    dwidp[2][2] = 1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),3.0/2.0)*(p1*x1*-2.0-p2*x2*2.0+p1*p1+p2*p2+x1*x1+x2*x2);

    if( any(isnan(dwidp[0])) || any(isnan(dwidp[1])) || any(isnan(dwidp[2])) ){
        dwidp = mat3(0.0);
        debugPrintfEXT("dwidp NaN");
    }

    if( isPointLight(light_idx) || isSpotLight(light_idx) ){
        float attenuation = cosOverR2; 
        float angularAttenuation = 1.0f;


        vec3 dattdp = vec3(0.0), dattdn = vec3(0.0);
        vec2 dattdio=vec2(0.0);
        if( isSpotLight(light_idx) ){
            const float cosTheta = dot(light.n_ws_norm.xyz, rayDirection );

            if( cosTheta < cos(light.inner_angle) && cosTheta > cos(light.outer_angle) ){
                const float light_angle_scale = 1.0 / (cos(light.inner_angle) - cos(light.outer_angle));
                const float light_angle_offset = -cos(light.outer_angle) * light_angle_scale;
                angularAttenuation = cosTheta * light.light_angle_scale + light.light_angle_offset;
		        angularAttenuation *= angularAttenuation;

                const float ia=light.inner_angle, oa=light.outer_angle, l1=light.n_ws_norm.x, l2=light.n_ws_norm.y, l3=light.n_ws_norm.z;
                dattdp[0] = (1.0/pow(cos(ia)-cos(oa),2.0)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),2.0)*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*(l1*(p2*p2)+l1*(p3*p3)+l1*(x2*x2)+l1*(x3*x3)-l2*p1*p2-l3*p1*p3-l1*p2*x2*2.0+l2*p1*x2+l2*p2*x1-l1*p3*x3*2.0+l3*p1*x3+l3*p3*x1-l2*x1*x2-l3*x1*x3)*2.0)/(l1*l1+l2*l2+l3*l3);
                dattdp[1] = (1.0/pow(cos(ia)-cos(oa),2.0)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),2.0)*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*(l2*(p1*p1)+l2*(p3*p3)+l2*(x1*x1)+l2*(x3*x3)-l1*p1*p2-l3*p2*p3+l1*p1*x2+l1*p2*x1-l2*p1*x1*2.0-l2*p3*x3*2.0+l3*p2*x3+l3*p3*x2-l1*x1*x2-l3*x2*x3)*2.0)/(l1*l1+l2*l2+l3*l3);
                dattdp[2] = (1.0/pow(cos(ia)-cos(oa),2.0)*1.0/pow(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0),2.0)*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*(l3*(p1*p1)+l3*(p2*p2)+l3*(x1*x1)+l3*(x2*x2)-l1*p1*p3-l2*p2*p3+l1*p1*x3+l1*p3*x1-l3*p1*x1*2.0+l2*p2*x3+l2*p3*x2-l3*p2*x2*2.0-l1*x1*x3-l2*x2*x3)*2.0)/(l1*l1+l2*l2+l3*l3);
                dattdn[0] = (1.0/pow(cos(ia)-cos(oa),2.0)*1.0/pow(l1*l1+l2*l2+l3*l3,2.0)*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*((l2*l2)*p1+(l3*l3)*p1-(l2*l2)*x1-(l3*l3)*x1-l1*l2*p2-l1*l3*p3+l1*l2*x2+l1*l3*x3)*2.0)/(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0));
                dattdn[1] = (1.0/pow(cos(ia)-cos(oa),2.0)*1.0/pow(l1*l1+l2*l2+l3*l3,2.0)*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*((l1*l1)*p2+(l3*l3)*p2-(l1*l1)*x2-(l3*l3)*x2-l1*l2*p1-l2*l3*p3+l1*l2*x1+l2*l3*x3)*2.0)/(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0));
                dattdn[2] = (1.0/pow(cos(ia)-cos(oa),2.0)*1.0/pow(l1*l1+l2*l2+l3*l3,2.0)*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*((l1*l1)*p3+(l2*l2)*p3-(l1*l1)*x3-(l2*l2)*x3-l1*l3*p1-l2*l3*p2+l1*l3*x1+l2*l3*x2)*2.0)/(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0));
                dattdio[0] = (sin(ia)*1.0/pow(cos(ia)-cos(oa),3.0)*pow(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3),2.0)*2.0)/((pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*(l1*l1+l2*l2+l3*l3));
                dattdio[1] = (sin(oa)*1.0/pow(cos(ia)-cos(oa),3.0)*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(oa)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*(l1*p1+l2*p2+l3*p3-l1*x1-l2*x2-l3*x3+cos(ia)*sqrt(pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*sqrt(l1*l1+l2*l2+l3*l3))*-2.0)/((pow(p1-x1,2.0)+pow(p2-x2,2.0)+pow(p3-x3,2.0))*(l1*l1+l2*l2+l3*l3));
            }
            else if( cosTheta < cos(light.outer_angle) ){
                angularAttenuation = 0.0;
            }
        }

        const float lightWeight = WATT_TO_RADIANT_INTENSITY(light.intensity) * attenuation;

        if(visible) {
            
            dWeightdp.position  = (lightWeight * angularAttenuation / cosOverR2) * dCosOverR2dp;
            dWeightdp.intensity = lightWeight * angularAttenuation / WATT_TO_RADIANT_INTENSITY(light.intensity) * WATT_TO_RADIANT_INTENSITY(1.0);
            dWeightdp.color     = lightWeight * angularAttenuation * vec3(1.0);

            if( isSpotLight(light_idx) ){
                dWeightdp.position += lightWeight * dattdp;
                dWeightdp.normal    = lightWeight * dattdn;
                dWeightdp.angles    = lightWeight * dattdio;
            }
        }

        
        
        
        
        
        
        
        

        
        

        
        
        
        
        
        
        
        
        

        
        
        
        

        
        

        
        

        
        
        
        
        
        
        
        
        
        
        
        
        

        
        

        
        
        
        
            
        
        

        
        
        
        
        

        
        
        
        
        

        
        
        
        
        
        
        
        
        
            
        
    }
    else if( isRectangleLight(light_idx) || isSquareLight(light_idx) ){
        const vec3 light_b_ws_norm = normalize(cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz));
        vec2 offsetAlongExtents = vec2(0.0); 
        offsetAlongExtents.x = dot( rayOrigin - light.pos_ws.xyz , light.t_ws_norm.xyz );
        offsetAlongExtents.y = dot( rayOrigin - light.pos_ws.xyz , light_b_ws_norm     );
        const float cosTheta = dot(rayDirection, light.n_ws_norm.xyz);
        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity) * 0.5f;

        vec3 dcodp, dcodn, dcodt, dcir2dp, dcir2dn, dcir2dt;
        geometricDerivativeRect(dcodp, dcodn, dcodt, dcir2dp, dcir2dn, dcir2dt,
            light.pos_ws.xyz, light.n_ws_norm.xyz, light.t_ws_norm.xyz, hd.hitX, hd.hitN,
            offsetAlongExtents.x, offsetAlongExtents.y
        );

        dWeightdp.position = fluxFactor * cosTheta*dcir2dp + fluxFactor * dCosOverR2dp; 
        dWeightdp.normal   = fluxFactor * cosTheta * cosOverR2 * dcir2dn + fluxFactor * dcodn;
        dWeightdp.tangent  = fluxFactor * cosTheta * cosOverR2 * dcir2dt + fluxFactor * dcodt;
        
        dWeightdp.intensity = fluxFactor * cosTheta / light.intensity;
        dWeightdp.color = vec3(1.0) * fluxFactor * cosTheta;
        
    }
}

vec3 evaluateBRDF(const in vec3 wi, const in vec3 wo, const in HitDataStruct hd){
    const BsdfScatterEvent e = { -wi, eLobeAll, hd.texNormal, hd.texNormal, hd.texMetallic, hd.texColor, hd.texRoughness, hd.texTransmission, hd.eta_ext, hd.eta_int, false };

    #if USE_GGX
        return metalroughness_eval(e, wo);
    #else
        return lambert_brdf_eval(e, wo); 
    #endif
}

void generateLightRay(inout vec3 rayOrigin, inout vec3 rayDirection, inout vec3 radiantFlux, const in uint nRays, const in uint light_idx){
    const Light_s light = light_buffer[light_idx];

    if( isPointLight(light_idx) ){
        
        rayDirection = normalize(sampleUnitSphereUniform(tea_nextFloat2(seed)));
        
        rayOrigin = light.pos_ws.xyz;

        
        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity) * M_4PI / float(nRays);
        radiantFlux = light.color * fluxFactor;
    }
    else if( isSpotLight(light_idx) ){
        const vec3 light_b_ws_norm = normalize(cross(light.t_ws_norm.xyz, light.n_ws_norm.xyz));

        
        rayDirection = normalize(tangentSpaceToWorldSpace(sampleUnitConeUniform(tea_nextFloat2(seed), cos(light.outer_angle)), light.t_ws_norm.xyz, light_b_ws_norm, light.n_ws_norm.xyz));
        
        rayOrigin = light.pos_ws.xyz;

        
        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity) * (M_2PI * (1.0-cos(light.outer_angle))) / float(nRays);

        const float cosTheta = dot(light.n_ws_norm.xyz, rayDirection);
		float angularAttenuation = 1.0;

        if( cosTheta < cos(light.inner_angle) && cosTheta > cos(light.outer_angle) ){
            const float light_angle_scale = 1.0 / (cos(light.inner_angle) - cos(light.outer_angle));
            const float light_angle_offset = -cos(light.outer_angle) * light_angle_scale;
            angularAttenuation = cosTheta * light.light_angle_scale + light.light_angle_offset;
		    angularAttenuation *= angularAttenuation;
        }

        radiantFlux = light.color * fluxFactor * angularAttenuation;
    }
    else if( isIesLight(light_idx) ){
        
        rayDirection = normalize(sampleUnitSphereUniform(tea_nextFloat2(seed)));
        
        rayOrigin = light.pos_ws.xyz;
        
        
        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity) * M_4PI / float(nRays);
        const float iesIntensity = evalIesLightBilinear(light_idx, rayDirection); 
        radiantFlux = light.color * iesIntensity * fluxFactor;
    }
    else if( isRectangleLight(light_idx) || isSquareLight(light_idx) ){
        const vec3 light_b_ws_norm = normalize(cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz));
        
        rayDirection = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), light.t_ws_norm.xyz, light_b_ws_norm, light.n_ws_norm.xyz));
        
        
        
        const vec2 relativeOffsetAlongExtents = tea_nextFloat2(seed) - 0.5f; 
        

        if( gl_LaunchIDEXT.x == 0 && gl_LaunchIDEXT.y == 0 )
            debugPrintfEXT(" area light: p (%.3g %.3g %.3g), n (%.3g %.3g %.3g), t (%.3g %.3g %.3g), d (%.3g %.3g)", 
                light.pos_ws.x,light.pos_ws.y,light.pos_ws.z, 
                light.n_ws_norm.x,light.n_ws_norm.y,light.n_ws_norm.z,
                light.t_ws_norm.x,light.t_ws_norm.y,light.t_ws_norm.z,
                light.dimensions.x,light.dimensions.y);

        rayOrigin = light.pos_ws.xyz + relativeOffsetAlongExtents.x * light.dimensions.x * light.t_ws_norm.xyz + relativeOffsetAlongExtents.y * light.dimensions.y * light_b_ws_norm;
        const float cosTheta = dot(rayDirection, light.n_ws_norm.xyz);

        
        
        
        
        
        const float fluxFactor = light.intensity  / float(nRays); 
        radiantFlux = light.color * fluxFactor * cosTheta; 

        
        
        
        
        
        
        
        
        
    }
}

bool generateSecondaryRay(inout vec3 rayOrigin, inout vec3 rayDirection, inout vec3 bounceThroughput, const in HitDataStruct hd){
    bounceThroughput = vec3(0.0);
    
    
    const vec3 wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereCosine(tea_nextFloat2(seed)), hd.hitN));
    
    const float pdf = PDF_UNIT_HEMISPHERE_COSINE( dot(wo,hd.hitN ) ); 

    const vec3 brdf = evaluateBRDF(rayDirection, wo, hd );
    if (all(equal(brdf, vec3(0)))) return false;

    
    bounceThroughput = brdf * dot(wo,hd.hitN) / pdf;
    rayOrigin = hd.hitX;
    rayDirection = wo;
    return true;
}

void symbolicBRDFderiv(out vec3 dbrdfdp[3], 
    const vec3 lp, const vec3 n, const vec3 wo, const vec3 albedo, 
    const float metallic, const float roughness, float eta_int, float eta_ext){
    const vec3 hp = vec3(0.0);
    const float alpha = roughness;
    const float alpha2 = alpha * alpha;
    const vec3 wi = normalize(lp - hp);
    const vec3 m = normalize(wi + wo);
    const float NdotWi = dot(n, wi);
    const float NdotWo = dot(n, wo);
    const float MdotWi = dot(m, wi);
    const float MdotWo = dot(m, wo);
    const float MdotN = dot(m, n);

    float cosT;
    const float fo = F_Dielectric(eta_ext, eta_int, abs(NdotWo), cosT);
    const float eta = eta_ext / eta_int;
	const float eta2 = eta * eta;

    const float re = fresnelDiffuseReflectance(eta_ext, eta_int);
	const float ri = 1.0f - eta2 * (1.0f - re);
	const vec3 diffuse = (1.0f - metallic) * eta2 * albedo * (1.0f - fo) / (M_PI * (1.0f - (PLASTIC_NONLINEAR ? albedo : vec3(1.0f)) * ri));

    const float n1 = n.x; const float n2 = n.y; const float n3 = n.z;
    const float wo1 = wo.x; const float wo2 = wo.y; const float wo3 = wo.z;
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float hp1 = hp.x; const float hp2 = hp.y; const float hp3 = hp.z;
    const float albedo1 = albedo.x; const float albedo2 = albedo.y; const float albedo3 = albedo.z;
    
    vec3 gradDiffuse = vec3(0.0f);
    {
        const float tinyEps = 0.0;
        #include "codegen/_codegen_diffuseBrdfDerivsWi.h"
    }
    vec3 gradSpecular = vec3(0.0f);
    {
        const float tinyEps = 0.0;
        #include "codegen/_codegen_specularBrdfDerivsWi.h"
    }

    if(any(isnan(gradDiffuse))) gradDiffuse = vec3(0.0f);
    if(any(isnan(gradSpecular)) || all(equal(wi + wo, vec3(0.0f))) || MdotN <= 0.0f || (MdotWo / NdotWo) <= 0 || (MdotWi / NdotWi) <= 0) gradSpecular = vec3(0.0f);
	if(NdotWi <= 0.0f || NdotWo <= 0.0f) { gradDiffuse = vec3(0.0f); gradSpecular = vec3(0.0f); }
    if((MdotWo / NdotWo) <= 0 || (MdotWi / NdotWi) <= 0 || MdotN <= 0 || all(equal(wi + wo, vec3(0.0f)))) gradSpecular = vec3(0.0f);
    
    dbrdfdp[0] = diffuse.x * gradDiffuse + gradSpecular;
    dbrdfdp[1] = diffuse.y * gradDiffuse + gradSpecular;
    dbrdfdp[2] = diffuse.z * gradDiffuse + gradSpecular;
}

void computeBrdfDerivative(inout vec3 dBrdfdWiR, inout vec3 dBrdfdWiG, inout vec3 dBrdfdWiB, const in vec3 wi, const in vec3 wo, const in HitDataStruct hd){
    #if USE_GGX
        
        vec3 wi_fd; const float fd_h=1e-2;
        wi_fd = wi + vec3(fd_h, 0.0, 0.0); wi_fd = normalize(wi_fd);
        vec3 brdf_x = evaluateBRDF(wi_fd,wo,hd);
        wi_fd = wi + vec3(0.0, fd_h, 0.0); wi_fd = normalize(wi_fd);
        vec3 brdf_y = evaluateBRDF(wi_fd,wo,hd);
        wi_fd = wi + vec3(0.0, 0.0, fd_h); wi_fd = normalize(wi_fd);
        vec3 brdf_z = evaluateBRDF(wi_fd,wo,hd);
        vec3 brdf_0 = evaluateBRDF(wi,wo,hd);
        
        vec3 dBrdfdWiR_fd = vec3( brdf_0.r-brdf_x.r , brdf_0.r-brdf_y.r , brdf_0.r-brdf_z.r ) / fd_h;
        vec3 dBrdfdWiG_fd = vec3( brdf_0.g-brdf_x.g , brdf_0.g-brdf_y.g , brdf_0.g-brdf_z.g ) / fd_h;
        vec3 dBrdfdWiB_fd = vec3( brdf_0.b-brdf_x.b , brdf_0.b-brdf_y.b , brdf_0.b-brdf_z.b ) / fd_h;
        
        

        
        vec3 dbrdfdp[3];
        symbolicBRDFderiv(dbrdfdp, -wi, hd.texNormal, wo, hd.texColor, hd.texMetallic, hd.texRoughness, hd.eta_int, hd.eta_ext);
        dBrdfdWiR = dbrdfdp[0]; dBrdfdWiG = dbrdfdp[1]; dBrdfdWiB = dbrdfdp[2];

        
        

    #else 
        dBrdfdWiR = dBrdfdWiG = dBrdfdWiB = vec3(0.0);
    #endif
}

void processLocalSample(const in HitDataStruct hd, const in vec3 wi, const in vec3 wo, const in vec3 radiantFlux, const in vec3 rayThroughput, const in uint numSamples, inout vec3 dOdFlux, inout vec3 dOdBrdf){
    dOdFlux = vec3(0.0);
    dOdBrdf = vec3(0.0);

    const vec3 brdf = evaluateBRDF(wi, wo, hd );
    if (all(equal(brdf, vec3(0)))) return;

    
    const vec3 dLdFlux   =                                brdf / float(numSamples) ;
    const vec3 dLdBrdf   =  radiantFlux * rayThroughput        / float(numSamples) ;


    float objFcnPartial[3][ENTRIES_PER_VERTEX];
    for(uint idx = 0u; idx < ENTRIES_PER_VERTEX; idx++) {
        objFcnPartial[0][idx] = target_radiance_weights_buffer[hd.idx[0]] * (radiance_buffer[hd.radBuffIdx[0] + idx] - target_radiance_buffer[hd.radBuffIdx[0] + idx]) ;
        objFcnPartial[1][idx] = target_radiance_weights_buffer[hd.idx[1]] * (radiance_buffer[hd.radBuffIdx[1] + idx] - target_radiance_buffer[hd.radBuffIdx[1] + idx]) ;
        objFcnPartial[2][idx] = target_radiance_weights_buffer[hd.idx[2]] * (radiance_buffer[hd.radBuffIdx[2] + idx] - target_radiance_buffer[hd.radBuffIdx[2] + idx]) ;
        
    }


    [[unroll, dependency_infinite]]
    for (int l = 0; l <= SPHERICAL_HARMONIC_ORDER; l++) {
        const float smoothing = 1.0f / (1.0f + SH_SMOOTHING * float((l * (l + 1)) * (l * (l + 1)))); 
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint shIdx = getSHindex(l, m);

            for(uint k = 0u; k < 3u; ++k){ 
                vec3 n_k, t_k;
                if      (k==0) { n_k=hd.n0; t_k=hd.t0; }
                else if (k==1) { n_k=hd.n1; t_k=hd.t1; }
                else if (k==2) { n_k=hd.n2; t_k=hd.t2; }

                
                if( abs(dot(n_k, n_k) - 1.0) > tinyEps ){
                    
                    n_k = normalize(n_k);
                }
                if( abs(dot(t_k, t_k) - 1.0) > tinyEps || abs(dot(n_k, t_k)) > tinyEps || any(isnan(t_k)) ){
                    
                    t_k = normalize(getPerpendicularVector(n_k)); 
                }

                float shWeight = 0.0f;
                if(dot(wo, n_k) > tinyEps) { shWeight = evalHSH(l, m, wo, n_k, t_k) * smoothing; }

                [[unroll, dependency_infinite]]
                for (uint i = 0u; i < 3u; i++) { 
                    if(GRAD_VIS_PARTIAL == GV_COMPLETE) {
                        dOdFlux[i] += dLdFlux[i] * objFcnPartial[k][shIdx * 3 + i] * shWeight * hd.bary[k];
                        
                        dOdBrdf[i] += dLdBrdf[i] * objFcnPartial[k][shIdx * 3 + i] * shWeight * hd.bary[k];
                    } else if(GRAD_VIS_PARTIAL == GV_LIGHT_GRAD) {
                        dOdFlux[i] += dLdFlux[i] * shWeight * hd.bary[k];
                        
                        dOdBrdf[i] += dLdBrdf[i] * shWeight * hd.bary[k];
                    } else if(GRAD_VIS_PARTIAL == GV_IMAGE_GRAD) {}
                }
            }
        }
    }
}



void main()
{
    const uint samplePointsN = gl_LaunchSizeEXT.y;
    const float sampleWeight = 1.0f / float(BOUNCE_SAMPLES);

    
    seed = tea_init(uint(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x), ubo.frameCount);
    
    vec3 outColorValue = vec3(ubo.bg_color);
    imageStore(output_image, ivec2(gl_LaunchIDEXT.xy), vec4(outColorValue, 1.0f));
    
    const vec2 sccs = getScreenCoordinates(vec2(0.5f));
	RayDesc rayDesc = getRay(sccs);

    rayDesc.flags = gl_RayFlagsNoneEXT;
    if (ubo.cull_mode == 1) rayDesc.flags = gl_RayFlagsCullFrontFacingTrianglesEXT;
    else if(ubo.cull_mode == 2) rayDesc.flags = gl_RayFlagsCullBackFacingTrianglesEXT;
    
    traceRayEXT(tlas, rayDesc.flags, 0xff, 0, 0, 0, rayDesc.origin, rayDesc.tmin, rayDesc.direction, rayDesc.tmax, 0);
    if ( ap.instanceIndex == -1) return; 

    HitDataStruct localSamplePoint;
    fetchHitPointData(localSamplePoint); 

    if (ubo.fd_grad_vis) {
        finiteDiff(true, localSamplePoint);
        return;
    }

    
    
    
    

    
    
    
    


    
    ParamDerivsDataStruct dFluxdp = ParamDerivsDataStruct(vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0.0f), 0.0f, vec3(0.0f));
    mat3 dwidp = mat3(0.0); 
    vec3 rayOrigin, rayDirection, radiantFlux, rayThroughput = vec3(1.0);
    Light_s light = light_buffer[ubo.light_deriv_idx];
    vec3 irradiance = evalLight( ubo.light_deriv_idx , localSamplePoint, rayDirection, rayOrigin);
    rayDirection = normalize( localSamplePoint.hitX - rayOrigin ); 



    

    
    RayDesc shadowRay;
    shadowRay.origin = localSamplePoint.hitX; 
    shadowRay.direction = normalize(-rayDirection);
    shadowRay.tmin = 1e-5f;
    shadowRay.tmax = length(rayDirection) - 1e-5f;
    traceRayEXT(tlas, gl_RayFlagsTerminateOnFirstHitEXT, 0xff, 0, 0, 0, shadowRay.origin, shadowRay.tmin, shadowRay.direction, shadowRay.tmax, 0);
    if (ap.instanceIndex != -1) {
        storeColorMap(0.0f, 0.0f);
        return; 
    }

    
    computeParametricDerivatives( dFluxdp, dwidp, localSamplePoint, ubo.light_deriv_idx, ap.instanceIndex == -1); 
                                                                                              
    
    


    

    vec3 dOdFlux = vec3(0.0);
    vec3 dOdp_brdf = vec3(0.0); 
    vec3 dFluxdBrdf = vec3(0.0);
    vec3 dOdBrdf_indirect = vec3(0.0); 
    vec3 dBrdfdWiR_indirect = vec3(0.0), dBrdfdWiG_indirect = vec3(0.0), dBrdfdWiB_indirect = vec3(0.0);

    uint depth=0; bool missed=false;
    HitDataStruct hd = localSamplePoint; 
    while( depth <= info.bounces && !missed){
        ++depth;

        
        const uint numSamples = max(BOUNCE_SAMPLES / depth, 1);
        [[unroll]]
        for(uint localSample = 0; localSample < numSamples; localSample++) {

            
            const vec3 wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), hd.hitN));
            
            if( dot(wo, hd.hitN) > tinyEps ){

                vec3 dOdFlux_i = vec3(0.0);
                vec3 dOdBrdf_i = vec3(0.0);
                processLocalSample( hd, rayDirection, wo, irradiance, rayThroughput, numSamples, dOdFlux_i, dOdBrdf_i );
                dOdFlux += rayThroughput * dOdFlux_i;

                if( depth==1 ){
                    vec3 dBrdfdWiR=vec3(0.0), dBrdfdWiG=vec3(0.0), dBrdfdWiB=vec3(0.0);
                    computeBrdfDerivative(dBrdfdWiR, dBrdfdWiG, dBrdfdWiB, rayDirection, wo, hd);
                    dOdp_brdf += (dOdBrdf_i.r * dBrdfdWiR + dOdBrdf_i.g * dBrdfdWiG + dOdBrdf_i.b * dBrdfdWiB) * dwidp;
                }else if( depth>1 ){
                    dOdBrdf_indirect += dOdFlux_i * dFluxdBrdf;
                }
            }
        }

        
        vec3 inRayDirection = rayDirection;
        vec3 bounceThroughput;
        if( generateSecondaryRay(rayOrigin, rayDirection, bounceThroughput, hd) ){
            rayThroughput *= bounceThroughput;

            
            if( depth == 1 ){
                computeBrdfDerivative(dBrdfdWiR_indirect, dBrdfdWiG_indirect, dBrdfdWiB_indirect, inRayDirection, rayDirection, hd);
                
                dFluxdBrdf  = irradiance * dot(rayDirection,hd.hitN) / PDF_UNIT_HEMISPHERE_COSINE( dot(rayDirection,hd.hitN ) ); 
            }else{
                dFluxdBrdf *= bounceThroughput;
            }
        }else{
            missed=true;
        }

        traceRayEXT(tlas, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, rayOrigin, 1e-6f, rayDirection, 1e+6f, 0);
        if( ap.instanceIndex == -1){ missed = true; }
        else{
            fetchHitPointData( hd );
            if( vtxArea[hd.idx[0]] < (tinyEps*tinyEps) || vtxArea[hd.idx[1]] < (tinyEps*tinyEps) || vtxArea[hd.idx[2]] < (tinyEps*tinyEps) ){ missed = true;}
        }
    }


    dOdp_brdf += (dOdBrdf_indirect.r * dBrdfdWiR_indirect + dOdBrdf_indirect.g * dBrdfdWiG_indirect + dOdBrdf_indirect.b * dBrdfdWiB_indirect) * dwidp;
    
    vec3 dOdP = dot(light_buffer[ubo.light_deriv_idx].color, dOdFlux) * dFluxdp.position +  dOdp_brdf;
    float dOdI = dot(light_buffer[ubo.light_deriv_idx].color, dOdFlux) * dFluxdp.intensity;

    
    
    float paramGrad = 0.0;
    switch(ubo.param_deriv_idx){
        case 0: paramGrad = dOdP.x; break; 
        case 1: paramGrad = dOdP.y; break; 
        case 2: paramGrad = dOdP.z; break; 
        case 3: paramGrad = dOdI; break; 
        
        
        
        
        
        
        
        
    }

    if (isnan(paramGrad)) return;
    storeColorMap(paramGrad, 0.0f);
}

#endif 









































































































