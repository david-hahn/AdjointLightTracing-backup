#define GLSL
#define BXDFS_EVAL_NO_COS
#include "defines.h"
#include "../utils/glsl/random.glsl"
#include "../utils/glsl/ray_tracing_utils.glsl"
#include "../utils/glsl/rendering_utils.glsl"

layout(constant_id = 0) const uint SPHERICAL_HARMONIC_ORDER = 0;
layout(constant_id = 1) const uint ENTRIES_PER_VERTEX = 3;
layout(constant_id = 2) const uint USE_UNPHYSICAL_NICE_PREVIEW = 0;


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

layout(binding = ADJOINT_DESC_INFO_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer info_storage_buffer { AdjointInfo_s info; };
layout(binding = ADJOINT_DESC_AREA_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer vertex_area_buffer { float vtxArea[]; };

#if EXEC_MODE == 0 
layout(binding = ADJOINT_DESC_RADIANCE_BUFFER_BINDING, set = ADJOINT_DESC_SET) buffer radiance_storage_buffer { float radiance[]; };
#define SH_DATA_BUFFER_ radiance
#elif EXEC_MODE == 1 || EXEC_MODE == 2 
layout(binding = ADJOINT_DESC_RADIANCE_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer radiance_storage_buffer { float objFcnPartial[]; };
layout(binding = ADJOINT_DESC_LIGHT_DERIVATIVES_BUFFER_BINDING, set = ADJOINT_DESC_SET) writeonly restrict buffer light_derivatives_buffer { LightGrads light_derivatives[]; };
layout(binding = ADJOINT_DESC_LIGHT_TEXTURE_DERIVATIVES_BUFFER_BINDING, set = ADJOINT_DESC_SET) writeonly restrict buffer light_texture_derivatives_buffer { double light_texture_derivatives[]; };
#define SH_DATA_BUFFER_ objFcnPartial
#endif

layout(location = 0) rayPayloadEXT AdjointPayload ap;

#include "../sphericalharmonics/sphericalharmonics.glsl"
#include "../utils/glsl/bsdf/bsdf.glsl"
#include "../utils/glsl/random.glsl"


uint seed;
const float tinyEps = 1e-5;

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

vec3 evaluateBRDF(const in vec3 wi, const in vec3 wo, const in HitDataStruct hd){
    const BsdfScatterEvent e = { -wi, eLobeAll, hd.texNormal, hd.texNormal, hd.texMetallic, hd.texColor, hd.texRoughness, hd.texTransmission, hd.eta_ext, hd.eta_int, false };

    #if USE_GGX
        return metalroughness_eval(e, wo);
    #else
        return lambert_brdf_eval(e, wo); 
    #endif
}


int tri_idx;
vec3 bary;
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

        
        
        
        
        
        const float fluxFactor = light.intensity * 0.5f / float(nRays); 
        radiantFlux = light.color * fluxFactor * cosTheta; 

        
        
        
        
        
        
        
        
        
    } else if(isTriangleMeshLight(light_idx)) {
		
		
        
		tri_idx = sampleRangeUniform(1, int(light.triangle_count), tea_nextFloat(seed)) - 1;
		bary = sampleUnitTriangleUniform(tea_nextFloat2(seed));
		
		uint idx_0 = 0;
		uint idx_1 = 0;
		uint idx_2 = 0;
		if(light.index_buffer_offset != uint(-1)){
			idx_0 = index_buffer[light.index_buffer_offset + (tri_idx * 3) + 0];
			idx_1 = index_buffer[light.index_buffer_offset + (tri_idx * 3) + 1];
			idx_2 = index_buffer[light.index_buffer_offset + (tri_idx * 3) + 2];
		} else {
			idx_0 = (tri_idx * 3) + 0;
			idx_1 = (tri_idx * 3) + 1;
			idx_2 = (tri_idx * 3) + 2;
		}
		mat4x4 mat = mat4(light.pos_ws, light.n_ws_norm, light.t_ws_norm, vec4(0,0,0,1));
		mat = transpose(mat);
		
        float area = 1;
		{
			vec3 v_0 = (mat * vertex_buffer[light.vertex_buffer_offset + idx_0].position).xyz;
			vec3 v_1 = (mat * vertex_buffer[light.vertex_buffer_offset + idx_1].position).xyz;
			vec3 v_2 = (mat * vertex_buffer[light.vertex_buffer_offset + idx_2].position).xyz;
			rayOrigin = (vec4(bary.x * v_0 + bary.y * v_1 + bary.z * v_2, 1)).xyz;

			vec3 n = cross((v_1 - v_0),(v_2 - v_0));
			float len = length(n);
			area = (abs(len/2.0f));

		}
		
		vec3 n_ws_norm;
		vec3 t_ws_norm;
		{
			mat3 normal_mat = mat3(transpose(inverse(mat)));
			vec3 n_0 = vertex_buffer[light.vertex_buffer_offset + idx_0].normal.xyz;
			vec3 n_1 = vertex_buffer[light.vertex_buffer_offset + idx_1].normal.xyz;
			vec3 n_2 = vertex_buffer[light.vertex_buffer_offset + idx_2].normal.xyz;
			n_ws_norm = normalize(normal_mat * (n_0 * bary.x + n_1 * bary.y + n_2 * bary.z));

			vec3 t_0 = vertex_buffer[light.vertex_buffer_offset + idx_0].tangent.xyz;
			vec3 t_1 = vertex_buffer[light.vertex_buffer_offset + idx_1].tangent.xyz;
			vec3 t_2 = vertex_buffer[light.vertex_buffer_offset + idx_2].tangent.xyz;
			t_ws_norm = normalize(normal_mat * (t_0 * bary.x + t_1 * bary.y + t_2 * bary.z));

            
            if( abs(dot(n_ws_norm, n_ws_norm) - 1.0) > tinyEps ){
                
                n_ws_norm = normalize(n_ws_norm);
            }
            if( abs(dot(t_ws_norm, t_ws_norm) - 1.0) > tinyEps || abs(dot(n_ws_norm, t_ws_norm)) > tinyEps || any(isnan(t_ws_norm)) ){
                
                t_ws_norm = normalize(getPerpendicularVector(n_ws_norm)); 
            }
		}
		
        vec3 color = light.color;
		if(light.texture_index != -1){
			vec2 uv_0 = vertex_buffer[light.vertex_buffer_offset + idx_0].texture_coordinates_0.xy;
			vec2 uv_1 = vertex_buffer[light.vertex_buffer_offset + idx_1].texture_coordinates_0.xy;
			vec2 uv_2 = vertex_buffer[light.vertex_buffer_offset + idx_2].texture_coordinates_0.xy;
			vec2 uv = (bary.x * uv_0 + bary.y * uv_1 + bary.z * uv_2);
			
			
            
            
            if(     uv.x<0.0 ) uv.x=0.0;
            else if(uv.x>1.0 ) uv.x=1.0; 
            if(     uv.y<0.0 ) uv.y=0.0;
            else if(uv.y>1 )   uv.y=1.0; 
            ivec2 idx, txsz = textureSize(texture_sampler[light.texture_index], 0);
            idx.x = int(floor( uv.x * txsz.x ));
            idx.y = int(floor( uv.y * txsz.y ));
            color *= texelFetch(texture_sampler[light.texture_index], idx, 0).xyz;
		}

        const vec3 light_b_ws_norm = normalize(cross(n_ws_norm, t_ws_norm));
        
        rayDirection = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), t_ws_norm, light_b_ws_norm, n_ws_norm));
        

        const float cosTheta = dot(rayDirection, n_ws_norm);
        const float fluxFactor = M_2PI * area * light.intensity * float(light.triangle_count) / float(nRays); 
        radiantFlux = color * fluxFactor * cosTheta; 
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

#if EXEC_MODE == 1 

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

void iesTexCoordDerivs(
    inout mat3x2 duvdp, inout mat3x2 duvdn, inout mat3x2 duvdt,
    const vec3 lp, const vec3 ld, const vec3 lt, const vec3 lb, const vec3 hp,
    const float minVAngle, const float maxVAngle, const float minHAngle, const float maxHAngle,
    const vec2 rangeUV
){
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float ld1 = ld.x; const float ld2 = ld.y; const float ld3 = ld.z;
    const float lt1 = lt.x; const float lt2 = lt.y; const float lt3 = lt.z;
    const float lb1 = lb.x; const float lb2 = lb.y; const float lb3 = lb.z;
    const float hp1 = hp.x; const float hp2 = hp.y; const float hp3 = hp.z;
    const float rangeUV1 = rangeUV.x; const float rangeUV2 = rangeUV.y;
    #include "codegen/_codegen_iesLightDerivs.h"
    if (any(isnan(duvdp[0])) || any(isnan(duvdp[1])) || any(isnan(duvdp[2])) || 
        any(isinf(duvdp[0])) || any(isinf(duvdp[1])) || any(isinf(duvdp[2]))) duvdp = mat3x2(0.0f);
    if (any(isnan(duvdn[0])) || any(isnan(duvdn[1])) || any(isnan(duvdn[2])) || 
        any(isinf(duvdn[0])) || any(isinf(duvdn[1])) || any(isinf(duvdn[2]))) duvdn = mat3x2(0.0f);
    if (any(isnan(duvdt[0])) || any(isnan(duvdt[1])) || any(isnan(duvdt[2])) || 
        any(isinf(duvdt[0])) || any(isinf(duvdt[1])) || any(isinf(duvdt[2]))) duvdt = mat3x2(0.0f);

}

void bilinearFilteringDerivs(inout vec2 dbfduv, const vec4 cd, const vec2 pixelUV)
{
    const float pixelUV1 = pixelUV.x; const float pixelUV2 = pixelUV.y;
    const float cd1 = cd.x; const float cd2 = cd.y; const float cd3 = cd.z; const float cd4 = cd.w;
    #include "codegen/_codegen_bilinearFilteringDerivs.h"
    if(any(isnan(dbfduv))) dbfduv = vec2(0.0f);
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

void computeParametricDerivatives(inout ParamDerivsDataStruct dFluxdp, inout mat3 dwidp, inout int texIDX, inout vec3 rayColor, const in vec3 rayOrigin, const in HitDataStruct hd, const in uint nRays, const in uint light_idx){
    const Light_s light = light_buffer[light_idx];

    rayColor = light.color; 
    texIDX = -1; 

    const float dist = length(hd.hitX - rayOrigin);
    const vec3 rayDirection = normalize( hd.hitX - rayOrigin );

    
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

    if( isPointLight(light_idx) ){
        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity) * M_4PI / float(nRays);
        dFluxdp.position = fluxFactor / cosOverR2 * dCosOverR2dp; 
        dFluxdp.intensity = fluxFactor / WATT_TO_RADIANT_INTENSITY(light.intensity) * WATT_TO_RADIANT_INTENSITY(1.0);
        dFluxdp.color = vec3(1.0) * fluxFactor;

        
        
        
        
        
        
        

        
        
        
        

        
        

        
        
        
        
        
        
        
        
        
        
        
        
        

        
            
        
        

        
        

        
        
        
        

        
        
        
        
        
        
        
            
        
    }
    else if( isSpotLight(light_idx) ){
        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity) * (M_2PI * (1.0-cos(light.outer_angle))) / float(nRays);

        const float cosTheta = dot(light.n_ws_norm.xyz, rayDirection );
		float angularAttenuation = 1.0;
        vec3 dattdp = vec3(0.0), dattdn = vec3(0.0);
        vec2 dattdio=vec2(0.0);

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

        
        dFluxdp.position = fluxFactor / cosOverR2 * dCosOverR2dp * angularAttenuation + fluxFactor * dattdp;
        dFluxdp.normal   = fluxFactor * dattdn;
        dFluxdp.angles   = fluxFactor * dattdio;
        dFluxdp.intensity= fluxFactor * angularAttenuation / WATT_TO_RADIANT_INTENSITY(light.intensity) * WATT_TO_RADIANT_INTENSITY(1.0);
        dFluxdp.color = vec3(1.0) * fluxFactor * angularAttenuation;
    }
    else if( isIesLight(light_idx) ){
        const uint texIdx = light_buffer[light_idx].texture_index;
        const vec2 texSize = textureSize(texture_sampler[texIdx], 0);
        const vec2 texelSize = 1.0f / texSize;
        const vec2 iesUV = getIesUVsBilinear(light_idx, rayDirection);
        const vec4 gather = textureGather(texture_sampler[texIdx], iesUV, 0); 
        const vec2 pixelUV = fract(iesUV*texSize-0.5f);
        const vec3 b_ws_norm = cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz);
        const vec2 rangeUV = vec2(1.0f) - texelSize;

        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity) * M_4PI / float(nRays);
        const float iesIntensity = texture(texture_sampler[texIdx], iesUV).r;

        
        
        
        
        
        

        
        
        

        
        
        
        
        
        

        
        
        
        
        
        

        
        
        
        
        
        

        
        
        
        

        
        
        
        

        
        
        
        
        
        
        
        
        

        
        
        
        
        
        
        
        

        
        mat3x2 duvdp, duvdn, duvdt;
        iesTexCoordDerivs(duvdp, duvdn, duvdt, 
            light.pos_ws.xyz, light.n_ws_norm.xyz, light.t_ws_norm.xyz, b_ws_norm, hd.hitX,
            light.min_vertical_angle, light.max_vertical_angle, light.min_horizontal_angle, light.max_horizontal_angle,
            rangeUV
        );
        

        vec2 dbfduv;
        bilinearFilteringDerivs(dbfduv, gather, pixelUV);
        dbfduv /= texelSize;
        

        
        dFluxdp.position = fluxFactor * dbfduv * duvdp + iesIntensity * (fluxFactor / cosOverR2) * dCosOverR2dp;
        dFluxdp.normal   = fluxFactor * dbfduv * duvdn;
        dFluxdp.tangent  = fluxFactor * dbfduv * duvdt;
        dFluxdp.intensity = iesIntensity * fluxFactor / WATT_TO_RADIANT_INTENSITY(light.intensity) * WATT_TO_RADIANT_INTENSITY(1.0);
        dFluxdp.color = vec3(1.0) * iesIntensity * fluxFactor;

    }
    else if( isRectangleLight(light_idx) || isSquareLight(light_idx) ){
        
        
        
        

        const vec3 light_b_ws_norm = normalize(cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz));
        vec2 offsetAlongExtents = vec2(0.0); 
        offsetAlongExtents.x = dot( rayOrigin - light.pos_ws.xyz , light.t_ws_norm.xyz );
        offsetAlongExtents.y = dot( rayOrigin - light.pos_ws.xyz , light_b_ws_norm     );
        const float cosTheta = dot(rayDirection, light.n_ws_norm.xyz);
        const float fluxFactor = light.intensity * 0.5f / float(nRays);

        vec3 dcodp, dcodn, dcodt, dcir2dp, dcir2dn, dcir2dt;
        geometricDerivativeRect(dcodp, dcodn, dcodt, dcir2dp, dcir2dn, dcir2dt,
            light.pos_ws.xyz, light.n_ws_norm.xyz, light.t_ws_norm.xyz, hd.hitX, hd.hitN,
            offsetAlongExtents.x, offsetAlongExtents.y
        );

        dFluxdp.position = fluxFactor * cosTheta / cosOverR2 * dcir2dp + fluxFactor * dcodp; 
        dFluxdp.normal   = fluxFactor * cosTheta / cosOverR2 * dcir2dn + fluxFactor * dcodn;
        dFluxdp.tangent  = fluxFactor * cosTheta / cosOverR2 * dcir2dt + fluxFactor * dcodt;
        
        dFluxdp.intensity = fluxFactor * cosTheta / light.intensity;
        dFluxdp.color = vec3(1.0) * fluxFactor * cosTheta;
        
        
        


        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    }  else if(isTriangleMeshLight(light_idx)) {
        
		uint idx_0 = 0;
		uint idx_1 = 0;
		uint idx_2 = 0;
		if(light.index_buffer_offset != uint(-1)){
			idx_0 = index_buffer[light.index_buffer_offset + (tri_idx * 3) + 0];
			idx_1 = index_buffer[light.index_buffer_offset + (tri_idx * 3) + 1];
			idx_2 = index_buffer[light.index_buffer_offset + (tri_idx * 3) + 2];
		} else {
			idx_0 = (tri_idx * 3) + 0;
			idx_1 = (tri_idx * 3) + 1;
			idx_2 = (tri_idx * 3) + 2;
		}
		mat4x4 mat = mat4(light.pos_ws, light.n_ws_norm, light.t_ws_norm, vec4(0,0,0,1));
		mat = transpose(mat);
        
        float area = 1;
		{
			vec3 v_0 = (mat * vertex_buffer[light.vertex_buffer_offset + idx_0].position).xyz;
			vec3 v_1 = (mat * vertex_buffer[light.vertex_buffer_offset + idx_1].position).xyz;
			vec3 v_2 = (mat * vertex_buffer[light.vertex_buffer_offset + idx_2].position).xyz;

			vec3 n = cross((v_1 - v_0),(v_2 - v_0));
			float len = length(n);
			area = (abs(len/2.0f));

		}
		
		vec3 n_ws_norm;
		vec3 t_ws_norm;
		{
			mat3 normal_mat = mat3(transpose(inverse(mat)));
			vec3 n_0 = vertex_buffer[light.vertex_buffer_offset + idx_0].normal.xyz;
			vec3 n_1 = vertex_buffer[light.vertex_buffer_offset + idx_1].normal.xyz;
			vec3 n_2 = vertex_buffer[light.vertex_buffer_offset + idx_2].normal.xyz;
			n_ws_norm = normalize(normal_mat * (n_0 * bary.x + n_1 * bary.y + n_2 * bary.z));

			vec3 t_0 = vertex_buffer[light.vertex_buffer_offset + idx_0].tangent.xyz;
			vec3 t_1 = vertex_buffer[light.vertex_buffer_offset + idx_1].tangent.xyz;
			vec3 t_2 = vertex_buffer[light.vertex_buffer_offset + idx_2].tangent.xyz;
			t_ws_norm = normalize(normal_mat * (t_0 * bary.x + t_1 * bary.y + t_2 * bary.z));

            
            if( abs(dot(n_ws_norm, n_ws_norm) - 1.0) > tinyEps ){
                
                n_ws_norm = normalize(n_ws_norm);
            }
            if( abs(dot(t_ws_norm, t_ws_norm) - 1.0) > tinyEps || abs(dot(n_ws_norm, t_ws_norm)) > tinyEps || any(isnan(t_ws_norm)) ){
                
                t_ws_norm = normalize(getPerpendicularVector(n_ws_norm)); 
            }
		}
		
        vec3 texColor = vec3(1.0);
        ivec2 idx, txsz;
		if(light.texture_index != -1){
			vec2 uv_0 = vertex_buffer[light.vertex_buffer_offset + idx_0].texture_coordinates_0.xy;
			vec2 uv_1 = vertex_buffer[light.vertex_buffer_offset + idx_1].texture_coordinates_0.xy;
			vec2 uv_2 = vertex_buffer[light.vertex_buffer_offset + idx_2].texture_coordinates_0.xy;
			vec2 uv = (bary.x * uv_0 + bary.y * uv_1 + bary.z * uv_2);
			
            
            
            
            if(     uv.x<0.0 ) uv.x=0.0;
            else if(uv.x>1.0 ) uv.x=1.0; 
            if(     uv.y<0.0 ) uv.y=0.0;
            else if(uv.y>1 )   uv.y=1.0; 
            txsz = textureSize(texture_sampler[light.texture_index], 0);
            idx.x = int(floor( uv.x * txsz.x ));
            idx.y = int(floor( uv.y * txsz.y ));
            texColor *= texelFetch(texture_sampler[light.texture_index], idx, 0).xyz;
		}
        rayColor = light.color * texColor; 

        const float cosTheta = dot(rayDirection, n_ws_norm);
        const float fluxFactor = M_2PI * area * light.intensity * float(light.triangle_count) / float(nRays); 
        
        dFluxdp.intensity = fluxFactor * cosTheta / light.intensity; 
        dFluxdp.color = texColor * fluxFactor * cosTheta; 
        dFluxdp.textureColor = light.color * fluxFactor * cosTheta; 
        texIDX = idx.x + txsz.x * idx.y;
    }
    
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

void addToLightDerivatives(const in vec3 dOdFlux, const in ParamDerivsDataStruct dFluxdp, const in vec3 dOdp_brdf, const in vec3 rayColor, const in int textureIdx, const in uint light_idx){
    const dvec3 dOdP = dot(rayColor, dOdFlux) * dFluxdp.position +  dOdp_brdf;
    const dvec3 dOdP_sum = subgroupAdd(dOdP);

    const dvec3 dOdN = dot(rayColor, dOdFlux) * dFluxdp.normal;
    const dvec3 dOdN_sum = subgroupAdd(dOdN);

    const dvec3 dOdT = dot(rayColor, dOdFlux) * dFluxdp.tangent;
    const dvec3 dOdT_sum = subgroupAdd(dOdT);

    const double dOdI = dot(rayColor, dOdFlux) * dFluxdp.intensity;
    const double dOdI_sum = subgroupAdd(dOdI);

    const dvec3 dOdC = dOdFlux * dFluxdp.color; 
    const dvec3 dOdC_sum = subgroupAdd(dOdC);

    const dvec2 dOdA = dot(rayColor, dOdFlux) * dFluxdp.angles;
    const dvec2 dOdA_sum = subgroupAdd(dOdA);

    if( subgroupElect() ){
        atomicAdd(light_derivatives[light_idx].dOdP[0], dOdP_sum[0]);
        atomicAdd(light_derivatives[light_idx].dOdP[1], dOdP_sum[1]);
        atomicAdd(light_derivatives[light_idx].dOdP[2], dOdP_sum[2]);

        atomicAdd(light_derivatives[light_idx].dOdN[0], dOdN_sum[0]);
        atomicAdd(light_derivatives[light_idx].dOdN[1], dOdN_sum[1]);
        atomicAdd(light_derivatives[light_idx].dOdN[2], dOdN_sum[2]);

        atomicAdd(light_derivatives[light_idx].dOdT[0], dOdT_sum[0]);
        atomicAdd(light_derivatives[light_idx].dOdT[1], dOdT_sum[1]);
        atomicAdd(light_derivatives[light_idx].dOdT[2], dOdT_sum[2]);

        atomicAdd(light_derivatives[light_idx].dOdIntensity, dOdI_sum);

        atomicAdd(light_derivatives[light_idx].dOdColor[0], dOdC_sum[0]);
        atomicAdd(light_derivatives[light_idx].dOdColor[1], dOdC_sum[1]);
        atomicAdd(light_derivatives[light_idx].dOdColor[2], dOdC_sum[2]);

        atomicAdd(light_derivatives[light_idx].dOdIAngle, dOdA_sum[0]);
        atomicAdd(light_derivatives[light_idx].dOdOAngle, dOdA_sum[1]);
    }

    if( textureIdx >= 0 ){ 
        const dvec3 dOdTC = dOdFlux * dFluxdp.textureColor; 
        atomicAdd(light_texture_derivatives[textureIdx*3  ], dOdTC[0]);
        atomicAdd(light_texture_derivatives[textureIdx*3+1], dOdTC[1]);
        atomicAdd(light_texture_derivatives[textureIdx*3+2], dOdTC[2]);
    }
}

#endif

#if EXEC_MODE == 0
void processLocalSample(const in HitDataStruct hd, const in vec3 wi, const in vec3 wo, const in vec3 radiantFlux, const in vec3 rayThroughput, const in uint numSamples){
#elif EXEC_MODE == 1
void processLocalSample(const in HitDataStruct hd, const in vec3 wi, const in vec3 wo, const in vec3 radiantFlux, const in vec3 rayThroughput, const in uint numSamples, inout vec3 dOdFlux, inout vec3 dOdBrdf){
#elif EXEC_MODE == 2
void processLocalSample(const in HitDataStruct hd, const in vec3 wi, const in vec3 wo, const in vec3 radiantFlux, const in vec3 rayThroughput, const in uint numSamples, inout vec3 dOdOrigin, const in vec3 dudp, const in vec3 dvdp){
#endif
    #if EXEC_MODE == 1 
    dOdFlux = vec3(0.0);
    dOdBrdf = vec3(0.0);
    #endif

    const vec3 brdf = evaluateBRDF(wi, wo, hd );
    if (all(equal(brdf, vec3(0)))) return;

    const vec3 localFlux = radiantFlux * rayThroughput * brdf / float(numSamples) ;
    #if EXEC_MODE == 1   
    const vec3 dLdFlux   =                               brdf / float(numSamples) ;
    const vec3 dLdBrdf   = radiantFlux * rayThroughput        / float(numSamples) ;
    #endif
    

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

                #if EXEC_MODE == 0 
                    [[unroll, dependency_infinite]]
                    for (uint i = 0u; i < 3u; i++) { 
                        atomicAdd(radiance[hd.radBuffIdx[k] + shIdx * 3 + i], localFlux[i] * shWeight * hd.bary[k] / vtxArea[hd.idx[k]] );
                    }
                #elif EXEC_MODE == 1 
                    [[unroll, dependency_infinite]]
                    for (uint i = 0u; i < 3u; i++) { 
                        dOdFlux[i] += dLdFlux[i] * objFcnPartial[hd.radBuffIdx[k] + shIdx * 3 + i] * shWeight * hd.bary[k] / vtxArea[hd.idx[k]];
                        
                        dOdBrdf[i] += dLdBrdf[i] * objFcnPartial[hd.radBuffIdx[k] + shIdx * 3 + i] * shWeight * hd.bary[k] / vtxArea[hd.idx[k]];
                    }
                #elif EXEC_MODE == 2 
                    for (uint i = 0u; i < 3u; i++) { 
                        vec3 duvdp;
                        if      (i==0) { duvdp = -dudp -dvdp; }
                        else if (i==1) { duvdp =  dudp      ; }
                        else if (i==2) { duvdp =        dvdp; }
                        dOdOrigin += localFlux[k] * objFcnPartial[hd.radBuffIdx[i] + shIdx * 3 + k] * shWeight / vtxArea[hd.idx[i]] * duvdp;
                    }
                #endif
            }
        }
    }
}

void main()
{
    const uint light_idx = uint(gl_LaunchIDEXT.z); 
    const uint nRays = (gl_LaunchSizeEXT.x * gl_LaunchSizeEXT.y);
    vec3 rayOrigin, rayDirection, radiantFlux, rayThroughput = vec3(1.0);
    HitDataStruct hd;

    
    seed = tea_init(uint(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x), info.seed); 
    
    
    
    generateLightRay(rayOrigin, rayDirection, radiantFlux, nRays, light_idx);

    #if EXEC_MODE == 1 
        vec3 dOdFlux = vec3(0.0);
        
        ParamDerivsDataStruct dFluxdp = ParamDerivsDataStruct(vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0.0f), 0.0f, vec3(0.0f));
        mat3 dwidp = mat3(0.0); 
        vec3 dOdp_brdf = vec3(0.0); 
        vec3 dFluxdBrdf = vec3(0.0);
        vec3 dOdBrdf_indirect = vec3(0.0); 
        vec3 dBrdfdWiR_indirect = vec3(0.0), dBrdfdWiG_indirect = vec3(0.0), dBrdfdWiB_indirect = vec3(0.0);
        vec3 rayColor = vec3(0.0); 
        int textureIdx = -1;
    #elif EXEC_MODE == 2 
        vec3 dOdOrigin = vec3(0.0), dudp = vec3(0.0), dvdp = vec3(0.0);
        mat3 dodp = mat3(1.0); 
    #endif

    uint depth=0; bool missed=false;
    while( depth <= info.bounces && !missed){
        ++depth;

        traceRayEXT(tlas, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, rayOrigin, 1e-6f, rayDirection, 1e+6f, 0);
        if( ap.instanceIndex == -1){ missed = true; }
        else{

            fetchHitPointData( hd );
            if( vtxArea[hd.idx[0]] < (tinyEps*tinyEps) || vtxArea[hd.idx[1]] < (tinyEps*tinyEps) || vtxArea[hd.idx[2]] < (tinyEps*tinyEps) ){ missed = true;}
            else{

                #if EXEC_MODE == 1 
                if( depth == 1 ){ 
                    computeParametricDerivatives( dFluxdp, dwidp, textureIdx, rayColor, rayOrigin, hd, nRays, light_idx );
                }

                #elif EXEC_MODE == 2 
                    dudp = vec3(0.0); dvdp = vec3(0.0);
                    const vec3 e1 = hd.v1 - hd.v0;
                    const vec3 e2 = hd.v2 - hd.v0;
                    const vec3 q = cross(rayDirection, e2);
                    const float a = dot(e1, q);
                    if( abs(a) > tinyEps ){ 
                        dudp = q / a;
                        dvdp = cross(e1, rayDirection) / a;
                    } 
                    dudp *= dodp; 
                    dvdp *= dodp;
                    
                    
                    dodp = outerProduct(hd.v0, (-dudp-dvdp)) + outerProduct(hd.v1, dudp) + outerProduct(hd.v2, dvdp);
                #endif

                vec3 localTexColor = hd.texColor;
                if( USE_UNPHYSICAL_NICE_PREVIEW == 1){
                    
                    hd.texColor = vec3(1.0);
                }

                
                const uint numSamples = max(BOUNCE_SAMPLES / depth, 1);
                [[unroll]]
                for(uint localSample = 0; localSample < numSamples; localSample++) {

                    
                    const vec3 wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), hd.hitN));
                    
                    if( dot(wo, hd.hitN) > tinyEps ){

                        #if EXEC_MODE == 0
                        processLocalSample( hd, rayDirection, wo, radiantFlux, rayThroughput, numSamples );
                        
                        #elif EXEC_MODE == 1
                        vec3 dOdFlux_i = vec3(0.0);
                        vec3 dOdBrdf_i = vec3(0.0);
                        processLocalSample( hd, rayDirection, wo, radiantFlux, rayThroughput, numSamples, dOdFlux_i, dOdBrdf_i );
                        dOdFlux += rayThroughput * dOdFlux_i;

                        if( depth==1 ){
                            vec3 dBrdfdWiR=vec3(0.0), dBrdfdWiG=vec3(0.0), dBrdfdWiB=vec3(0.0);
                            computeBrdfDerivative(dBrdfdWiR, dBrdfdWiG, dBrdfdWiB, rayDirection, wo, hd);
                            dOdp_brdf += (dOdBrdf_i.r * dBrdfdWiR + dOdBrdf_i.g * dBrdfdWiG + dOdBrdf_i.b * dBrdfdWiB) * dwidp;
                        }else if( depth>1 ){
                            dOdBrdf_indirect += dOdFlux_i * dFluxdBrdf;
                        }

                        #elif EXEC_MODE == 2
                        processLocalSample( hd, rayDirection, wo, radiantFlux, rayThroughput, numSamples, dOdOrigin, dudp, dvdp );
                        #endif
                    }
                }
                
                hd.texColor = localTexColor;

                
                #if EXEC_MODE == 1
                vec3 inRayDirection = rayDirection;
                #endif
                vec3 bounceThroughput;
                if( generateSecondaryRay(rayOrigin, rayDirection, bounceThroughput, hd) ){
                    rayThroughput *= bounceThroughput;
                    #if EXEC_MODE == 1
                    
                    if( depth == 1 ){
                        computeBrdfDerivative(dBrdfdWiR_indirect, dBrdfdWiG_indirect, dBrdfdWiB_indirect, inRayDirection, rayDirection, hd);
                        dFluxdBrdf  = radiantFlux * dot(rayDirection,hd.hitN) / PDF_UNIT_HEMISPHERE_COSINE( dot(rayDirection,hd.hitN ) ); 
                    }else{
                        dFluxdBrdf *= bounceThroughput;
                    }
                    #endif
                }else{
                    missed=true;
                }
            }
        }
    }

    #if EXEC_MODE == 1 
        dOdp_brdf += (dOdBrdf_indirect.r * dBrdfdWiR_indirect + dOdBrdf_indirect.g * dBrdfdWiG_indirect + dOdBrdf_indirect.b * dBrdfdWiB_indirect) * dwidp;
        addToLightDerivatives(dOdFlux, dFluxdp, dOdp_brdf, rayColor, textureIdx, light_idx);
    #elif EXEC_MODE == 2 
        atomicAdd(light_derivatives[light_idx].dOdP[0], dOdOrigin[0]);
        atomicAdd(light_derivatives[light_idx].dOdP[1], dOdOrigin[1]);
        atomicAdd(light_derivatives[light_idx].dOdP[2], dOdOrigin[2]);
    #endif

}
