#ifndef GLSL_DERIV_FUNCTIONS
#define GLSL_DERIV_FUNCTIONS

void geometricDerivativePoint(out vec3 dp, const vec3 lp, const vec3 n, const vec3 hp)
{
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float n1 = n.x; const float n2 = n.y; const float n3 = n.z;
    const float hp1 = hp.x; const float hp2 = hp.y; const float hp3 = hp.z;
    const float tinyEps = 1e-2;

    #include "codegen/_codegen_pointLightDerivs.h"
    if(any(isnan(dp))) dp = vec3(0.0f);
}

void geometricDerivativeSpot(out vec3 dp, out vec3 dn, out float dia, out float doa, const vec3 lp, const vec3 n, const vec3 hp, const vec3 ld, const float ia, const float oa)
{
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float ld1 = ld.x; const float ld2 = ld.y; const float ld3 = ld.z;
    const float n1 = n.x; const float n2 = n.y; const float n3 = n.z;
    const float hp1 = hp.x; const float hp2 = hp.y; const float hp3 = hp.z;
    const float tinyEps = 1e-2;

    #include "codegen/_codegen_spotLightDerivs.h"

    if(any(isnan(dp))) dp = vec3(0.0f);
    if(any(isnan(dn))) dn = vec3(0.0f);
    if(isnan(dia)) dia = 0.0f;
    if(isnan(doa)) doa = 0.0f;
}

void geometricDerivativeRect(
    out vec3 dp, out vec3 dn, out vec3 dt,
    const vec3 lp, const vec3 ld, const vec3 lt, const vec3 lb, const vec3 hp, const vec3 n,
    const float dimX, const float dimY, const float dx, const float dy
) 
{
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float ld1 = ld.x; const float ld2 = ld.y; const float ld3 = ld.z;
    const float lt1 = lt.x; const float lt2 = lt.y; const float lt3 = lt.z;
    const float lb1 = lb.x; const float lb2 = lb.y; const float lb3 = lb.z;
    const float hp1 = hp.x; const float hp2 = hp.y; const float hp3 = hp.z;
    const float n1 = n.x; const float n2 = n.y; const float n3 = n.z;
    #include "codegen/_codegen_rectLightDerivs.h"
    if(any(isnan(dp))) dp = vec3(0.0f);
    if(any(isnan(dn))) dn = vec3(0.0f);
    if(any(isnan(dt))) dt = vec3(0.0f);
}

void iesTexCoordDerivs(
    out mat3x2 duvdp, out mat3x2 duvdn, out mat3x2 duvdt,
    const vec3 lp, const vec3 ld, const vec3 lt, const vec3 lb, const vec3 hp,
    const float minVAngle, const float maxVAngle, const float minHAngle, const float maxHAngle,
    const vec2 rangeUV
){
    const float tinyEps = 1e-2;
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

void bilinearFilteringDerivs(out vec2 dbfduv, const vec4 cd, const vec2 pixelUV)
{
    const float pixelUV1 = pixelUV.x; const float pixelUV2 = pixelUV.y;
    const float cd1 = cd.x; const float cd2 = cd.y; const float cd3 = cd.z; const float cd4 = cd.w;
    #include "codegen/_codegen_bilinearFilteringDerivs.h"
    if(any(isnan(dbfduv))) dbfduv = vec2(0.0f);
}

void brdfLightPosDerivs(out vec3 dbrdfdp[3], 
    const vec3 lp, const vec3 hp, const vec3 n, const vec3 wi, const vec3 albedo, 
    const float metallic, const float roughness, float eta_int, float eta_ext){
    const float alpha = roughness * roughness;
    const float alpha2 = alpha * alpha;
    const vec3 wo = normalize(lp - hp);
    const vec3 m = normalize(wi + wo);
    const float NdotWi = dot(n, wi);
    const float NdotWo = dot(n, wo);
    const float MdotWi = dot(m, wi);
    const float MdotWo = dot(m, wo);
    const float MdotN = dot(m, n);

    float cosT;
    const float fi = F_Dielectric(eta_ext, eta_int, abs(NdotWi), cosT);
    const float eta = eta_ext / eta_int;
	const float eta2 = eta * eta;
	const float re = fresnelDiffuseReflectance(eta_ext, eta_int);
	const float ri = 1.0f - eta2 * (1.0f - re);
	const vec3 diffuse = (1.0f - metallic) * (1.0f - fi) * eta2 * albedo / (M_PI * (1.0f - (PLASTIC_NONLINEAR ? albedo : vec3(1.0f)) * ri));

    const float n1 = n.x; const float n2 = n.y; const float n3 = n.z;
    const float wi1 = wi.x; const float wi2 = wi.y; const float wi3 = wi.z;
    const float lp1 = lp.x; const float lp2 = lp.y; const float lp3 = lp.z;
    const float hp1 = hp.x; const float hp2 = hp.y; const float hp3 = hp.z;
    const float albedo1 = albedo.x; const float albedo2 = albedo.y; const float albedo3 = albedo.z;

    float tinyEps = 0;

    vec3 gradDiffuse = vec3(0.0f);
    {
#include "codegen/_codegen_diffuseBrdfDerivs.h"
    }
    vec3 gradSpecular = vec3(0.0f);
    {
#include "codegen/_codegen_specularBrdfDerivs.h"
    }

    if(any(isnan(gradDiffuse))) gradDiffuse = vec3(0.0f);
    if(any(isnan(gradSpecular)) || all(equal(wi + wo, vec3(0.0f))) || MdotN <= 0.0f || (MdotWo / NdotWo) <= 0 || (MdotWi / NdotWi) <= 0) gradSpecular = vec3(0.0f);
	if(NdotWi <= 0.0f || NdotWo <= 0.0f) { gradDiffuse = vec3(0.0f); gradSpecular = vec3(0.0f); }
    if((MdotWo / NdotWo) <= 0 || (MdotWi / NdotWi) <= 0) gradSpecular = vec3(0.0f);
    
    gradSpecular *= fi;
    
    dbrdfdp[0] = diffuse.x * gradDiffuse + gradSpecular;
    dbrdfdp[1] = diffuse.y * gradDiffuse + gradSpecular;
    dbrdfdp[2] = diffuse.z * gradDiffuse + gradSpecular;
    
    
    
    
}

#endif /* GLSL_DERIV_FUNCTIONS */
