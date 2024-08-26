#define GLSL
#define BXDFS_EVAL_NO_COS
#include "defines.h"
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

#include "../utils/glsl/bsdf/bsdf.glsl"
#include "../utils/glsl/random.glsl"

layout(binding = ADJOINT_DESC_INFO_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer info_storage_buffer { AdjointInfo_s info; };

#if EXEC_MODE == 0 
layout(binding = ADJOINT_DESC_RADIANCE_BUFFER_BINDING, set = ADJOINT_DESC_SET) buffer radiance_storage_buffer { float radiance[]; };
#define SH_DATA_BUFFER_ radiance
#elif EXEC_MODE == 1 
layout(binding = ADJOINT_DESC_RADIANCE_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer radiance_storage_buffer { float objFcnPartial[]; };
layout(binding = ADJOINT_DESC_LIGHT_DERIVATIVES_BUFFER_BINDING, set = CONV_LIGHT_BUFFER_SET) writeonly restrict buffer light_derivatives_buffer { LightGrads light_derivatives[]; };
#define SH_DATA_BUFFER_ objFcnPartial
#endif

layout(binding = ADJOINT_DESC_AREA_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer vertex_area_buffer { float vtxArea[]; };
layout(binding = ADJOINT_DESC_TRIANGLE_BUFFER_BINDING, set = ADJOINT_DESC_SET) readonly restrict buffer triangle_buffer { uvec2 triBuffer[]; };

layout(location = 0) rayPayloadEXT AdjointPayload ap;

#include "../sphericalharmonics/sphericalharmonics.glsl"

uint seed;
const float tinyEps = 1e-5;

vec3 evaluateBRDF(const in vec3 wi, const in vec3 wo, const in HitDataStruct hd){
    const BsdfScatterEvent e = { wi, eLobeAll, hd.texNormal, hd.texNormal, hd.texMetallic, hd.texColor, hd.texRoughness, hd.texTransmission, hd.eta_ext, hd.eta_int, false };

    #if USE_GGX
        return metalroughness_eval(e, wo);
    #else
        return lambert_brdf_eval(e, wo); 
    #endif
}

vec2 relativeOffsetAlongExtents;
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

void computeParametricDerivatives(inout ParamDerivsDataStruct dWeightdp, out mat3 dwidp, const in HitDataStruct hd,  const in uint light_idx){
    const Light_s light = light_buffer[light_idx];
    vec3 rayOrigin = light.pos_ws.xyz;
    if (isRectangleLight(light_idx) || isSquareLight(light_idx)) {
        const vec3 light_b_ws_norm = normalize(cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz));
        rayOrigin = light.pos_ws.xyz + relativeOffsetAlongExtents.x * light.dimensions.x * light.t_ws_norm.xyz + relativeOffsetAlongExtents.y * light.dimensions.y * light_b_ws_norm;
    }
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
        
        dWeightdp.position  = lightWeight * angularAttenuation / cosOverR2 * dCosOverR2dp;
        dWeightdp.intensity = lightWeight * angularAttenuation / WATT_TO_RADIANT_INTENSITY(light.intensity) * WATT_TO_RADIANT_INTENSITY(1.0);
        dWeightdp.color     = lightWeight * angularAttenuation * vec3(1.0);

        if( isSpotLight(light_idx) ){
            dWeightdp.position += lightWeight * dattdp;
            dWeightdp.normal    = lightWeight * dattdn;
            dWeightdp.angles    = lightWeight * dattdio;
        }
    }else if( isRectangleLight(light_idx) || isSquareLight(light_idx) ){
        
        
        
        

        const vec3 light_b_ws_norm = normalize(cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz));
        vec2 offsetAlongExtents = vec2(0.0); 
        offsetAlongExtents.x = dot( rayOrigin - light.pos_ws.xyz , light.t_ws_norm.xyz );
        offsetAlongExtents.y = dot( rayOrigin - light.pos_ws.xyz , light_b_ws_norm     );
        const float cosTheta = dot(rayDirection, light.n_ws_norm.xyz);
        const float fluxFactor = WATT_TO_RADIANT_INTENSITY(light.intensity);

        vec3 dcodp, dcodn, dcodt, dcir2dp, dcir2dn, dcir2dt;
        geometricDerivativeRect(dcodp, dcodn, dcodt, dcir2dp, dcir2dn, dcir2dt,
            light.pos_ws.xyz, light.n_ws_norm.xyz, light.t_ws_norm.xyz, hd.hitX, hd.hitN,
            offsetAlongExtents.x, offsetAlongExtents.y
        );

        dWeightdp.position = fluxFactor * cosTheta * dcir2dp + fluxFactor * cosOverR2 * dcodp; 
        dWeightdp.normal   = fluxFactor * cosTheta * dcir2dn + fluxFactor * cosOverR2 * dcodn;
        dWeightdp.tangent  = fluxFactor * cosTheta * dcir2dt + fluxFactor * cosOverR2 * dcodt;
        dWeightdp.intensity = cosTheta * cosOverR2 * WATT_TO_RADIANT_INTENSITY(1);
        dWeightdp.color = vec3(1.0) * fluxFactor * cosTheta * cosOverR2;
    } else if( isIesLight(light_idx) ){
        /* something is wrong here - disabled for now 
        const uint texIdx = light_buffer[light_idx].texture_index;
        const vec2 texSize = textureSize(texture_sampler[texIdx], 0);
        const vec2 texelSize = 1.0f / texSize;
        const vec2 iesUV = getIesUVsBilinear(light_idx, rayDirection);
        const vec4 gather = textureGather(texture_sampler[texIdx], iesUV, 0); 
        const vec2 pixelUV = fract(iesUV*texSize-0.5f);
        const vec3 b_ws_norm = cross(light.n_ws_norm.xyz, light.t_ws_norm.xyz);
        const vec2 rangeUV = vec2(1.0f) - texelSize;

        const float iesIntensity = texture(texture_sampler[texIdx], iesUV).r;
        const float fluxFactor = (light.intensity);

        
        mat3x2 duvdp, duvdn, duvdt;
        iesTexCoordDerivs(duvdp, duvdn, duvdt, 
            light.pos_ws.xyz, light.n_ws_norm.xyz, light.t_ws_norm.xyz, b_ws_norm, hd.hitX,
            light.min_vertical_angle, light.max_vertical_angle, light.min_horizontal_angle, light.max_horizontal_angle,
            rangeUV
        );

        vec2 dbfduv;
        bilinearFilteringDerivs(dbfduv, gather, pixelUV);
        dbfduv /= texelSize;

        dWeightdp.position = fluxFactor * dbfduv * duvdp * cosOverR2 + fluxFactor * iesIntensity * dCosOverR2dp;
        dWeightdp.normal   = fluxFactor * dbfduv * duvdn * cosOverR2;
        dWeightdp.tangent  = fluxFactor * dbfduv * duvdt * cosOverR2;
        dWeightdp.intensity = iesIntensity * fluxFactor / WATT_TO_RADIANT_INTENSITY(light.intensity) * WATT_TO_RADIANT_INTENSITY(1.0);
        dWeightdp.color = vec3(1.0) * iesIntensity * fluxFactor; /**/
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

        vec3 brdf_x = evaluateBRDF(wi_fd, wo, hd);
        wi_fd = wi + vec3(0.0, fd_h, 0.0); wi_fd = normalize(wi_fd);
        vec3 brdf_y = evaluateBRDF(wi_fd, wo, hd);
        wi_fd = wi + vec3(0.0, 0.0, fd_h); wi_fd = normalize(wi_fd);
        vec3 brdf_z = evaluateBRDF(wi_fd, wo, hd);
        vec3 brdf_0 = evaluateBRDF(wi, wo, hd);
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

void addToLightDerivatives(const in vec3 dOdFlux, const in ParamDerivsDataStruct dFluxdp, const in vec3 dOdp_brdf, const in uint light_idx){
    const dvec3 dOdP = dot(light_buffer[light_idx].color, dOdFlux) * dFluxdp.position + dOdp_brdf;
    const dvec3 dOdP_sum = subgroupAdd(dOdP);

    const dvec3 dOdN = dot(light_buffer[light_idx].color, dOdFlux) * dFluxdp.normal;
    const dvec3 dOdN_sum = subgroupAdd(dOdN);

    const dvec3 dOdT = dot(light_buffer[light_idx].color, dOdFlux) * dFluxdp.tangent;
    const dvec3 dOdT_sum = subgroupAdd(dOdT);

    const double dOdI = dot(light_buffer[light_idx].color, dOdFlux) * dFluxdp.intensity;
    const double dOdI_sum = subgroupAdd(dOdI);

    const dvec3 dOdC = dOdFlux * dFluxdp.color; 
    const dvec3 dOdC_sum = subgroupAdd(dOdC);

    const dvec2 dOdA = dot(light_buffer[light_idx].color, dOdFlux) * dFluxdp.angles;
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
}
#endif 


#if EXEC_MODE == 0
void processLocalSample(const in HitDataStruct hd, const vec3 wo, const float sampleWeight, const vec3 incidentRadiance, const vec3 incidentDirection){
#elif EXEC_MODE == 1
void processLocalSample(const in HitDataStruct hd, const vec3 wo, const float sampleWeight, const vec3 incidentRadiance, const vec3 incidentDirection, inout vec3 dOdRadiance, const bool firstHit, inout vec3 dOdwi){
#endif
    if( vtxArea[hd.idx[0]] < (tinyEps*tinyEps) || vtxArea[hd.idx[1]] < (tinyEps*tinyEps) || vtxArea[hd.idx[2]] < (tinyEps*tinyEps) ) return;
    if( dot(wo, hd.hitN) < tinyEps ) return;

    const vec3 brdf = evaluateBRDF(incidentDirection, wo, hd);

    #if EXEC_MODE == 0 
    if( any(isnan(incidentRadiance)) ) debugPrintfEXT("sample radiance is nan");

    vec3 localFlux = sampleWeight * incidentRadiance * brdf;

    #elif EXEC_MODE == 1 
    vec3 dFlux = sampleWeight * brdf;

    vec3 dBRDF = sampleWeight * incidentRadiance;
    vec3 dBrdfdWiR = vec3(0.0), dBrdfdWiG = vec3(0.0), dBrdfdWiB = vec3(0.0);
    if( firstHit ){ computeBrdfDerivative(dBrdfdWiR, dBrdfdWiG, dBrdfdWiB, incidentDirection, wo, hd); }
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
                        dOdRadiance[i] += dFlux[i] * objFcnPartial[hd.radBuffIdx[k] + shIdx * 3 + i] * shWeight * hd.bary[k] / vtxArea[hd.idx[k]];
                    }
                    if( firstHit ){
                        dOdwi += dBrdfdWiR * dBRDF[0] * objFcnPartial[hd.radBuffIdx[k] + shIdx * 3 + 0] * hd.bary[k] / vtxArea[hd.idx[k]];
                        dOdwi += dBrdfdWiG * dBRDF[1] * objFcnPartial[hd.radBuffIdx[k] + shIdx * 3 + 1] * hd.bary[k] / vtxArea[hd.idx[k]];
                        dOdwi += dBrdfdWiB * dBRDF[2] * objFcnPartial[hd.radBuffIdx[k] + shIdx * 3 + 2] * hd.bary[k] / vtxArea[hd.idx[k]];
                    }
                #endif
            }
        }
    }
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

vec3 evalLight(const uint index, const in HitDataStruct hd, inout vec3 f_l_dir_norm){
    if( index >= info.light_count ) return vec3(0.0);
	Light_s light = light_buffer[index];

	vec3 f_l_dir;
	float f_l_dist;
    float lightWeight = 1.0;

    vec3 lightPos = light.pos_ws.xyz;

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
            angularAttenuation = 2.0*cosTheta;
        } else if(isIesLight(index)) {
            attenuation *= evalIesLightBilinear(index, -f_l_dir_norm);
        }

		lightWeight *= WATT_TO_RADIANT_INTENSITY(light.intensity) * attenuation * angularAttenuation;
        if(isRectangleLight(index) || isSquareLight(index)) lightWeight *= 0.5f;
	}
	
	if(/*!hd.is_double_sided &&*/ dot(hd.hitN,f_l_dir_norm)  <= tinyEps){ return vec3(0.0); }

    traceRayEXT(tlas, gl_RayFlagsTerminateOnFirstHitEXT, 0xff, 0, 0, 0, hd.hitX, 0.01f, f_l_dir_norm, f_l_dist - tinyEps, 0);
    if (ap.instanceIndex != -1) return vec3(0.0); 
	return light.color * lightWeight;
}

uint sampleLight(const in HitDataStruct hd){
	if(info.light_count == 0) return -1;
    return gl_LaunchIDEXT.z;
}

void main()
{
    
    seed = tea_init(uint(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x), info.seed);
    const uint globalTriIdx = gl_LaunchIDEXT.x;

    const uint samplePointsN = gl_LaunchSizeEXT.y;
    

    HitDataStruct localSamplePoint;
    
    
    vec3 bary = sampleUnitTriangleUniform(tea_nextFloat2(seed));
    

    fetchTriangleData(triBuffer[globalTriIdx].x, triBuffer[globalTriIdx].y, bary, localSamplePoint);
    float triArea = length(cross(localSamplePoint.p1 - localSamplePoint.p0, localSamplePoint.p2 - localSamplePoint.p0)) * 0.5f;

    if( USE_UNPHYSICAL_NICE_PREVIEW == 1){
        
        localSamplePoint.texColor = vec3(1.0);
    }

    

    if( triArea < tinyEps ) return;
    if( vtxArea[localSamplePoint.idx[0]] < (tinyEps*tinyEps) || vtxArea[localSamplePoint.idx[1]] < (tinyEps*tinyEps) || vtxArea[localSamplePoint.idx[2]] < (tinyEps*tinyEps) ) return;
    
    const float sampleWeight = triArea / float(samplePointsN) / float(BOUNCE_SAMPLES);

    /**
    vec3 directIncidentDirection = vec3(0.0);
    uint lightIdx = sampleLight(localSamplePoint);
    vec3 directIncidentRadiance = evalLight(lightIdx, localSamplePoint, directIncidentDirection);

    

    if( length(directIncidentRadiance) > tinyEps ){
        #if EXEC_MODE == 1
        vec3 dOdDirectRadiance = vec3(0.0);
        vec3 dOdwi = vec3(0.0);
        mat3 dwidp = mat3(0.0);
        ParamDerivsDataStruct dLightdp = ParamDerivsDataStruct(vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0.0f), 0.0f, vec3(0.0f));

        computeParametricDerivatives( dLightdp, dwidp, localSamplePoint, lightIdx );
        
        #endif

        for(uint k=0; k < BOUNCE_SAMPLES; ++k ){
            
            vec3 wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereUniform(tea_nextFloat2(seed)), localSamplePoint.hitN));
            
            
            #if EXEC_MODE == 0 
            processLocalSample(localSamplePoint, wo, sampleWeight, directIncidentRadiance, directIncidentDirection);
            #elif EXEC_MODE == 1 
            processLocalSample(localSamplePoint, wo, sampleWeight, directIncidentRadiance, directIncidentDirection, dOdDirectRadiance, true, dOdwi);

            if( any(isnan(dOdDirectRadiance)) ) debugPrintfEXT("dOdDirectRadiance is nan");
            if( any(isnan(dOdwi)) ) debugPrintfEXT("dOdwi is nan");
            #endif
        }

        #if EXEC_MODE == 1
        vec3 dOdp_brdf = dOdwi * dwidp;
        if( any(isnan(dOdp_brdf)) ) debugPrintfEXT("dOdp_brdf is nan");

        addToLightDerivatives(dOdDirectRadiance, dLightdp, dOdp_brdf, lightIdx);
        #endif
    }
    /**/

    /**
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


    #if EXEC_MODE == 1 
    vec3 dOdIndirectRadiance = vec3(0.0);
    vec3 unused = vec3(0.0);

    for(uint k=0; k < BOUNCE_SAMPLES; ++k ){
        
        
        

        const vec3 wo = indirectLocalDirections[k];
        processLocalSample(localSamplePoint, wo, sampleWeight, vec3(1.0), indirectIncidentDirection, dOdIndirectRadiance, false, unused);
    }
    #endif

    uint depth = 0; bool missed = false;
    while( depth < info.bounces && !missed ){
        ++depth;

        
        traceRayEXT(tlas, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, hd.hitX, 1e-6f, rayDirection,  1e+6f, 0);

        if( ap.instanceIndex == -1){ missed = true; }
        else{
            fetchTriangleData(ap.customInstanceID + ap.geometryIndex, ap.primitiveIndex, computeBarycentric(ap.hitAttribute), hd); 
            
            
            vec3 remoteIncidentDirection = vec3(0.0);
            uint remoteLightIdx = sampleLight(hd);
            vec3 remoteIncidentRadiance = evalLight(remoteLightIdx, hd, remoteIncidentDirection);

            if( length(remoteIncidentRadiance) > tinyEps ){
                
                const vec3 remoteBRDF = evaluateBRDF(remoteIncidentDirection, -rayDirection, hd);         

                #if EXEC_MODE == 0 
                indirectIncidentRadiance += rayImportance * remoteBRDF * remoteIncidentRadiance;

                #elif EXEC_MODE == 1 
                
                mat3 dwidp = mat3(0.0);
                ParamDerivsDataStruct dLightdp = ParamDerivsDataStruct(vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec2(0.0f), 0.0f, vec3(0.0f));
                computeParametricDerivatives( dLightdp, dwidp, hd, remoteLightIdx );

                vec3 dBrdfdWiR = vec3(0.0), dBrdfdWiG = vec3(0.0), dBrdfdWiB = vec3(0.0);
                computeBrdfDerivative(dBrdfdWiR, dBrdfdWiG, dBrdfdWiB, remoteIncidentDirection, -rayDirection, hd);
                
                vec3 dOdL = rayImportance * remoteIncidentRadiance * dOdIndirectRadiance;
                vec3 dOdp_brdf = (dOdL.r * dBrdfdWiR + dOdL.g * dBrdfdWiG + dOdL.b * dBrdfdWiB ) * dwidp;

                addToLightDerivatives( rayImportance * remoteBRDF * dOdIndirectRadiance, dLightdp, dOdp_brdf, remoteLightIdx);
                #endif
            }

            
            
            
            const vec3 nextRayDirection = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereCosine(tea_nextFloat2(seed)), hd.hitN));
            
            pdf = PDF_UNIT_HEMISPHERE_COSINE( dot(nextRayDirection,hd.hitN ) ); 

            
            const vec3 indirectBRDF = evaluateBRDF(nextRayDirection, -rayDirection, hd);

            rayImportance *= indirectBRDF * dot(nextRayDirection, hd.texNormal) / pdf;
            rayDirection = nextRayDirection;
        }
    }

    #if EXEC_MODE == 0 
    for(uint k=0; k < BOUNCE_SAMPLES; ++k ){
        
        
        

        const vec3 wo = indirectLocalDirections[k];
        processLocalSample(localSamplePoint, wo, sampleWeight, indirectIncidentRadiance, indirectIncidentDirection);
    }
    #endif
    /**/
}
