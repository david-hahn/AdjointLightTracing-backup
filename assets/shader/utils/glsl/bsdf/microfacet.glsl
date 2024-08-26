

#ifndef GLSL_BSDF_MICROFACET
#define GLSL_BSDF_MICROFACET

#ifndef GLSL_RENDERING_UTILS
#include "../rendering_utils.glsl"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717959f
#endif
#ifndef M_INV_PI
#define M_INV_PI 0.318309886183791f
#endif
#ifndef M_INV_2PI
#define M_INV_2PI 0.159154943091895f
#endif
#ifndef M_INV_4PI
#define M_INV_4PI 0.0795774715459477f
#endif
#ifndef M_PI_DIV_2
#define M_PI_DIV_2 1.5707963267949f
#endif
#ifndef M_PI_DIV_4
#define M_PI_DIV_4 0.785398163397448f
#endif





float shininessToRoughness(const float shininess) {
    return sqrt(2.0f / (shininess + 2.0f));
}
float roughnessToShininess(const float roughness) {
    return (2.0f / (roughness * roughness)) - 2.0f;
}






float D_PDF_M_TO_WO(const float woDotM /*VdotM*/) {
    if (woDotM == 0.0f) return 0.0f;
    return 0.25f / abs(woDotM);
}

float D_PDF_M_TO_WO(const float wiDotM, const float woDotM, const float eta_i, const float eta_o) {
    const float a = eta_i * wiDotM + eta_o * woDotM;
    if (a == 0.0f) return 0.0f;
    return (eta_o * eta_o * abs(woDotM)) / (a * a);
}




/*
** Normal Distribution Function (NDF)
*/
/* Isotropic */


float D_GGX(const float MdotN, const float alpha) {
    if (MdotN <= 0) return 0;
    const float alpha2 = alpha * alpha;
    const float cos_theta = MdotN;
    const float cos_theta2 = cos_theta * cos_theta;
    const float tan_theta2 = (1.0f - cos_theta2) / cos_theta2;

    const float A = M_PI * cos_theta2 * cos_theta2 * (alpha2 + tan_theta2) * (alpha2 + tan_theta2);
    return alpha2 / A;
}

float D_GGX_PDF(const float MdotN, const float alpha) {
    return D_GGX(MdotN, alpha) * MdotN;
}





float D_GTR(const float MdotN, const float alpha, const float gamma) {
    if (MdotN <= 0) return 0;
    const float alpha2 = alpha * alpha;
    const float t = 1.0f + (alpha2 - 1.0f) * MdotN * MdotN;
    return alpha2 / (M_PI * pow(t, gamma));
}
float D_GTR2(const float MdotN, const float alpha) {
    return D_GTR(MdotN, alpha, 2.0f);
}
#define D_GGX D_GTR2
float D_GTR2_PDF(const float MdotN, const float alpha) {
    return D_GTR2(MdotN, alpha) * MdotN;
}

vec2 D_GGX_Importance_Sample(const vec2 xi, const float alpha) {

    
    return vec2(atan(alpha * sqrt(xi.x) / sqrt(1.0f - xi.x)),
        
        M_2PI * xi.y);
    
    
    
    
}
float D_GGX_Anisotropic(const vec3 N, const vec3 H, const float roughness_x, const float roughness_y)
{
    return 0;
}

/*
**	Geometry Masking and Shadowing Function
*/

float G1_GGX(const float Ndot_, const float alpha) {
    const float alpha2 = alpha * alpha;
    const float cos_theta2 = Ndot_ * Ndot_;
    const float tan_theta2 = (1.0f - cos_theta2) / cos_theta2;
    const float lambda = (-1.0f + sqrt(1.0f + alpha2 * tan_theta2)) * 0.5f;
    return 1.0f / (1.0f + lambda);
}


float G1_GGX(const float Mdot_, const float Ndot_, const float alpha) {
    if ((Mdot_ / Ndot_) <= 0) return 0;
    const float alpha2 = alpha * alpha;
    const float cos_theta2 = Ndot_ * Ndot_;
    const float tan_theta2 = (1.0f - cos_theta2) / cos_theta2;
    return 2.0f / (1.0f + sqrt(1.0f + alpha2 * tan_theta2));
}
float G_Smith_GGX(const float MdotV, const float MdotL, const float NdotV, const float NdotL, const float alpha) {
    const float g1v = G1_GGX(MdotV, NdotV, alpha);
    const float g1l = G1_GGX(MdotL, NdotL, alpha);
    return g1v * g1l;
}











/*
**	BRDF
**  https:
*/






float BSDF_Cook_Torrance(const float NdotL, const float NdotV) {
    
    return 1.0f / (M_PI * abs(NdotL) * abs(NdotV));
}

float BSDF_Torrance_Sparrow(const float wiDotHr, const float woDotHr) {
    if (wiDotHr * woDotHr == 0) return 0;
    return 1.0f / (4.0f * abs(wiDotHr) * abs(woDotHr));
}
float BSDF_Torrance_Sparrow(const float wiDotHt, const float woDotHt, const float wiDotN, const float woDotN,
    const float eta_i, const float eta_o) {
    const float a = (abs(wiDotHt) * abs(woDotHt)) / (abs(wiDotN) * abs(woDotN));
    const float b = (eta_i * wiDotHt + eta_o * woDotHt);
    if (b == 0.0f) return 0.0f;
    return a * ((eta_o * eta_o) / (b * b));
}




#define BSDF_GGX BSDF_Torrance_Sparrow

/*
** Complete GGX
*/

float /*brdf*/ ggx_eval(const vec3 wi, const vec3 wo, const vec3 n, const vec3 h, const float roughness) {
    const float nDotH = dot(h, n);
    const float nDotWi = dot(n, wi);
    const float nDotWo = dot(n, wo);
    const float wiDotH = dot(h, wi);
    const float woDotH = dot(h, wo);

    const float d_ggx = D_GTR2(nDotH, roughness);
    const float g_ggx = G_Smith_GGX(wiDotH, woDotH, nDotWi, nDotWo, roughness);

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
    if (isnan(wiDotH)) debugPrintfEXT("NAN: ggx_eval wiDotH %f", wiDotH);
    if (isnan(woDotH)) debugPrintfEXT("NAN: ggx_eval woDotH %f", woDotH);
    if (isnan(nDotWo)) debugPrintfEXT("NAN: ggx_eval nDotWo %f", nDotWo);
    if (isnan(nDotWi)) debugPrintfEXT("NAN: ggx_eval nDotWi %f", nDotWi);
    if (isnan(d_ggx)) debugPrintfEXT("NAN: ggx_eval d_ggx");
    if (isnan(g_ggx)) debugPrintfEXT("NAN: ggx_eval g_ggx");
#endif
    return d_ggx * g_ggx;
}
vec3 /*m*/ ggx_sample(const vec3 wi, const vec3 n, const float roughness, const vec2 rand) {
    const vec2 spherical = D_GGX_Importance_Sample(rand, roughness);
    vec3 m = normalize(sphericalToCartesian(vec3(1, spherical.xy)));
    return normalize(tangentSpaceToWorldSpace(m, n));
}
float /*pdf*/ ggx_pdf(vec3 n, vec3 m, const vec3 wi, float roughness) {
    if (dot(n, m) <= 0) return 0;
    const float d_ggx = D_GTR2(dot(n, m), roughness);
    const float pdf = d_ggx * dot(m, n);
    return max(pdf, 0.0f);
    
    
}

float ggx_weight(const float wiDotM, const float woDotM, const float wiDotN, const float woDotN, const float mDotN, const float roughness) {
    const float G = G_Smith_GGX(wiDotM, woDotM, wiDotN, woDotN, roughness);
    const float denom = abs(wiDotN) * abs(mDotN);
    if(denom == 0.0f) return 0.0f;
    return (abs(wiDotM) * G) / denom;
}























































#endif /* GLSL_BSDF_MICROFACET */