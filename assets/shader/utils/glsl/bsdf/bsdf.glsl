

#ifndef GLSL_BSDF_BSDF
#define GLSL_BSDF_BSDF


#ifndef PLASTIC_NONLINEAR
#define PLASTIC_NONLINEAR false
#endif

#ifndef GLSL_BSDF_FRESNEL
#include "fresnel.glsl"
#endif
#ifndef GLSL_BSDF_MICROFACET
#include "microfacet.glsl"
#endif
#ifndef GLSL_RENDERING_UTILS
#include "../rendering_utils.glsl"
#endif
#ifndef GLSL_RAY_TRACING_UTILS
#include "../ray_tracing_utils.glsl"
#endif

#ifndef __cplusplus
    #ifndef REF
        #define REF(type) inout type
    #endif
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

#ifndef isLobe
#define isLobe(lobe, bit) (lobe & bit > 0)
#endif
const uint eLobeDiffuse					= 0x00000001u;
const uint eLobeSpecularReflection		= 0x00000002u;
const uint eLobeSpecularTransmission	= 0x00000004u;
const uint eLobeGlossyReflection		= 0x00000008u;
const uint eLobeGlossyTransmission		= 0x00000010u;
const uint eLobeAll						= 0xFFFFFFFFu;

struct BsdfScatterEvent {
	vec3	wi;				
	uint	lobe;			
	vec3	geo_n;			
	vec3	n;				
	float	metallic;		
	vec3	albedo;
	float	roughness;		
	float	transmission;	
	float	eta_ext;		
	float	eta_int;		
	bool	thin;			
};

/*********************/
/*      LAMBERT      */
/*********************/

vec3 /*brdf*/ lambert_brdf_eval(const BsdfScatterEvent e, const vec3 wo){
	vec3 result = e.albedo * M_INV_PI;
#ifndef BXDFS_EVAL_NO_COS
	result *= max(dot(e.n,wo), 0.0f);
#endif
    return result;
}



vec3 lambert_disney_brdf_eval(const float wiDotN, const float woDotN, const float VdotH, const vec3 albedo, const float roughness){
    const float fd90 = 0.5 + 2 * roughness * VdotH * VdotH;
    const float fnl = 1 + (fd90 - 1) * pow(1 - wiDotN, 5);
    const float fnv = 1 + (fd90 - 1) * pow(1 - woDotN, 5);
    return albedo * M_INV_PI * fnl * fnv;
}

float /*pdf*/ lambert_brdf_pdf(const BsdfScatterEvent e, const vec3 wo){
	return max(0.0f, dot(wo, e.n)) * M_INV_PI;
}

bool lambert_brdf_sample(const BsdfScatterEvent e, const vec2 rand, REF(vec3) wo, REF(float) pdf, REF(vec3) weight){
	wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereCosine(rand), e.n));
    pdf = lambert_brdf_pdf(e, wo);
    
    weight = e.albedo;
	if (dot(wo,e.n) <= 0.0f) return false;
    return true;
}

/*********************/
/*     CONDUCTOR     */
/*********************/

vec3 conductor_eval(const BsdfScatterEvent e, const vec3 wo) {
	if(e.roughness == 0.0f) return vec3(0.0f);
	const vec3 m = halfvector(e.wi, wo);
	const float wiDotN = dot(e.wi, e.n);
	const float woDotN = dot(wo, e.n);
	const float wDotM = dot(e.wi, m); /* in case of reflection -> wiDotM == woDotM */
	
	if (dot(e.wi, e.geo_n) <= 0.0f || dot(wo, e.geo_n) <= 0.0f || wDotM <= 0.0f) return vec3(0);
	if (all(equal(e.wi + wo, vec3(0.0f)))) return vec3(0.0f);

	const vec3 eta = vec3(1.657460, 0.880369, 0.521229);
	const vec3 k = vec3(9.223869, 6.269523, 4.837001);
	
	const vec3 fi = F_Schlick(wDotM, e.albedo);

	float ggx = ggx_eval(e.wi, wo, e.n, m, e.roughness) * BSDF_GGX(wiDotN, woDotN);
#ifndef BXDFS_EVAL_NO_COS
	ggx *= max(woDotN, 0.0f);
#endif
	const vec3 result = ggx * fi;
#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (any(isnan(fi))) debugPrintfEXT("NAN: conductor_eval fi");
	if (any(isnan(result))) debugPrintfEXT("NAN: conductor_eval result");
#endif
	if (any(isnan(result))) return vec3(0.0f);
	return result;
}

float conductor_pdf(const BsdfScatterEvent e, const vec3 wo) {
	if(e.roughness == 0.0f) return 0.0f;

	const vec3 m = halfvector(e.wi, wo);
	const float wiDotN = dot(e.wi, e.n);
	const float woDotN = dot(wo, e.n);
	const float wDotM = dot(e.wi, m); /* in case of reflection -> wiDotM == woDotM */
	
	if (wDotM <= 0.0f) return 0.0f;
	
	if (dot(e.wi, e.geo_n) <= 0.0f) return 0.0f;
	if (dot(wo, e.geo_n) <= 0.0f) return 0.0f;
	
	if (all(equal(e.wi + wo, vec3(0.0f)))) return 0.0f;

	const float pdf = ggx_pdf(e.n, m, e.wi, e.roughness) * D_PDF_M_TO_WO(wDotM);
#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (isnan(pdf)) debugPrintfEXT("NAN: conductor_pdf pdf");
#endif
	if (isnan(pdf)) return 0.0f;
	return pdf;
}

bool conductor_sample(const BsdfScatterEvent e, const vec2 rand, REF(vec3) wo, REF(float) pdf, REF(vec3) weight) {
	const float wiDotN = dot(e.wi, e.n);
	const float sampleRoughness = (1.2f - 0.2f * sqrt(abs(wiDotN))) * e.roughness;
	const bool rZero = (e.roughness == 0.0f);
	const vec3 m = rZero ? e.n : ggx_sample(e.wi, e.n, sampleRoughness, rand.xy);
	wo = normalize(reflect(-e.wi, m));

	const float woDotN = dot(wo, e.n);
	const float wDotM = dot(e.wi, m); /* in case of reflection -> wiDotM == woDotM */

	if (wDotM <= 0.0f) return false;
	
	if (dot(e.wi, e.geo_n) <= 0.0f) return false;
	if (dot(wo, e.geo_n) <= 0.0f) return false;
	
	if (all(equal(e.wi + wo, vec3(0.0f)))) return false;

	const vec3 eta = vec3(1.657460, 0.880369, 0.521229);
	const vec3 k = vec3(9.223869, 6.269523, 4.837001);
	
	const vec3 fi = F_Schlick(wDotM, e.albedo);

	pdf = 0.0f;
	weight = fi;
	if(!rZero) {
		pdf = ggx_pdf(e.n, m, e.wi, e.roughness) * D_PDF_M_TO_WO(wDotM);
		weight *= ggx_weight(wDotM, wDotM, wiDotN, woDotN, dot(m, e.n), e.roughness);
		
		
	}
#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (isnan(pdf)) debugPrintfEXT("NAN: conductor_sample pdf");
	if (any(isnan(weight))) debugPrintfEXT("NAN: conductor_sample weight");
#endif
	if (isinf(pdf) || isnan(pdf) || any(isnan(weight))) return false;
	return true;
}

/*********************/
/*      PLASTIC      */
/*********************/

/* material that consists of layering of a reflective part and a diffuse part */

vec3 plastic_eval(BsdfScatterEvent e, const vec3 wo) {
	const bool rZero = (e.roughness == 0.0f);
	if(rZero) return vec3(0.0f);

	const float wiDotN = dot(e.wi, e.n);
	const float woDotN = dot(wo, e.n);
	const bool reflection = (wiDotN * woDotN) >= 0.0f;

	vec3 m;
	if (reflection) m = halfvector(e.wi, wo);
	else m = halfvector(e.wi, wo, e.eta_ext, e.eta_int);
	
	m *= sign(dot(m, e.n));

	const float mDotN = dot(m, e.n);
	const float wiDotM = dot(e.wi, m);
	const float woDotM = dot(wo, m);

	if (mDotN <= 0.0f) return vec3(0);
	
	if (dot(e.wi, e.geo_n) <= 0.0f) return vec3(0);
	if (dot(wo, e.geo_n) <= 0.0f) return vec3(0);
	
	
	
	if (all(equal(e.wi + wo, vec3(0.0f)))) return vec3(0);

	float cosThetaT;
	const float fo = F_Dielectric(e.eta_ext, e.eta_int, abs(woDotN), cosThetaT);
	const float fi = F_Dielectric(e.eta_ext, e.eta_int, abs(wiDotN), cosThetaT);
	const float specular = !rZero ? ggx_eval(e.wi, wo, e.n, m, e.roughness) * BSDF_GGX(wiDotN, woDotN) : 0.0f;
	
	const float eta = e.eta_ext / e.eta_int;
	const float eta2 = eta * eta;
	const float re = fresnelDiffuseReflectance(e.eta_ext, e.eta_int);
	const float ri = 1.0f - eta2 * (1.0f - re);
	
	const vec3 diffuse = eta2 * e.albedo * (1.0f - fo) / (M_PI * (1.0f - (PLASTIC_NONLINEAR ? e.albedo : vec3(1.0f)) * ri));
	vec3 result = (1.0f - fi) * diffuse + (fi) * specular;
#ifndef BXDFS_EVAL_NO_COS
	result *= max(woDotN, 0.0f);
#endif

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (any(isnan(result))) debugPrintfEXT("NAN: plastic_eval result");
#endif
	if (any(isnan(result))) return vec3(0.0f);
	return result;
}

float plastic_pdf(BsdfScatterEvent e, const vec3 wo) {
	const bool rZero = (e.roughness == 0.0f);
	if(rZero) return 0.0f;

	const float wiDotN = dot(e.wi, e.n);
	const float woDotN = dot(wo, e.n);
	const bool reflection = (wiDotN * woDotN) >= 0.0f;

	vec3 m;
	if (reflection) m = halfvector(e.wi, wo);
	else m = halfvector(e.wi, wo, e.eta_ext, e.eta_int);
	
	m *= sign(dot(m, e.n));

	const float mDotN = dot(m, e.n);
	const float wiDotM = dot(e.wi, m);
	const float woDotM = dot(wo, m);
	if (mDotN <= 0.0f) return 0;
	
	if (dot(e.wi, e.geo_n) <= 0.0f) return 0;
	if (dot(wo, e.geo_n) <= 0.0f) return 0;
	
	
	
	if (all(equal(e.wi + wo, vec3(0.0f)))) return 0;

	float cosThetaT;
	const float fi = F_Dielectric(e.eta_ext, e.eta_int, abs(wiDotN), cosThetaT);
	const float pdf_ggx = ggx_pdf(e.n, m, e.wi, e.roughness);

	const float pdf_specular = !rZero ? pdf_ggx * D_PDF_M_TO_WO(woDotM) : 0.0f;
	const float pdf_diffuse = abs(woDotN) * M_INV_PI;
	const float pdf = mix(pdf_diffuse, pdf_specular, fi);

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (isnan(pdf)) debugPrintfEXT("NAN: plastic_pdf pdf");
#endif
	if (isnan(pdf)) return 0.0f;
	return pdf;
}

bool plastic_sample(BsdfScatterEvent e, const vec3 rand0, const vec2 rand1, 
	REF(vec3) wo, REF(float) pdf, REF(vec3) weight) {

	const float wiDotN = dot(e.wi, e.n);
	const float sampleRoughness = (1.2f - 0.2f*sqrt(abs(wiDotN))) * e.roughness;
	const bool rZero = (e.roughness == 0.0f);
	const vec3 m = rZero ? e.n : ggx_sample(e.wi, e.n, sampleRoughness, rand0.xy);
	const float wiDotM = dot(e.wi, m);

	float cosThetaT;
	const float fi = F_Dielectric(e.eta_ext, e.eta_int, abs(wiDotN), cosThetaT);
	const bool reflection = rand0.z < fi;

	if (reflection) wo = reflectionDir(e.wi, m);
	else wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereCosine(rand1.xy), e.n));

	const float woDotN = dot(wo, e.n);
	const float woDotM = dot(wo, m);
	const float mDotN = dot(m, e.n);
	if (mDotN <= 0.0f) return false;
	
	if (all(equal(e.wi + wo, vec3(0.0f)))) return false;
	
	if (dot(e.wi, e.geo_n) <= 0.0f) return false;
	if (dot(wo, e.geo_n) <= 0.0f) return false;
	
	

	pdf = rZero ? 0.0f : 1.0f;
	weight = vec3(1.0f);
	if (reflection) /* specular */ {
		if(!rZero) {
			pdf *= ggx_pdf(e.n, m, e.wi, e.roughness) * D_PDF_M_TO_WO(woDotM);
			weight = vec3(ggx_weight(wiDotM, woDotM, wiDotN, woDotN, mDotN, e.roughness));
			
			
			pdf *= fi;
		}
	} else /* diffuse */ {
		pdf *= abs(woDotN) * M_INV_PI;
		weight = e.albedo;
		
		
		pdf *= (1.0f - fi);


		
		const float fo = F_Dielectric(e.eta_ext, e.eta_int, abs(woDotN), cosThetaT);
		const float eta = e.eta_ext / e.eta_int;
		const float eta2 = eta * eta;
		const float re = fresnelDiffuseReflectance(e.eta_ext, e.eta_int);
		const float ri = 1.0f - eta2 * (1.0f - re);
		weight *= eta2 * (1.0f - fo) / (1.0f - (PLASTIC_NONLINEAR ? e.albedo : vec3(1.0f)) * ri);
	}

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (isnan(pdf)) debugPrintfEXT("NAN: plastic_sample pdf");
	if (any(isnan(weight))) debugPrintfEXT("NAN: plastic_sample weight");
#endif
	if (isinf(pdf) || isnan(pdf) || any(isnan(weight))) return false;
	return true;
}

/*********************/
/*     DIELECTRIC    */
/*********************/




vec3 /*brdf*/ dielectric_eval(BsdfScatterEvent e, const vec3 wo) {
	const bool rZero = (e.roughness == 0.0f);
	if(rZero) return vec3(0.0f);

	
	if(dot(e.wi, e.geo_n) < 0) {
		e.n = -e.n;
		e.geo_n = -e.geo_n;
		float temp = e.eta_ext;
		e.eta_ext = e.eta_int;
		e.eta_int = temp;
	}
	const float wiDotN = dot(e.wi, e.n);

	const float woDotN = dot(wo, e.n);
	const bool reflection = (wiDotN * woDotN) >= 0.0f;

	vec3 m;
	if (reflection) m = halfvector(e.wi, wo);
	else m = halfvector(e.wi, wo, e.eta_ext, e.eta_int);
	
	m *= sign(dot(m, e.n));

	const float mDotN = dot(m, e.n);
	const float wiDotM = dot(e.wi, m);
	const float woDotM = dot(wo, m);

	if (mDotN <= 0.0f) return vec3(0);
	if (reflection && dot(wo, e.geo_n) <= 0) return vec3(0);
	if (!reflection && dot(wo, e.geo_n) >= 0) return vec3(0);
	

	float cosThetaT;
	const float fi = F_Dielectric(e.eta_ext, e.eta_int, abs(wiDotM), cosThetaT);
	const float ggx = ggx_eval(e.wi, wo, e.n, m, e.roughness);
	vec3 result = e.albedo;
	if (reflection) {
		result *= fi * vec3(ggx * BSDF_GGX(wiDotN, woDotN));
	} else {
		result *= (1.0f - fi) * vec3(ggx * BSDF_GGX(wiDotM, woDotM, wiDotN, woDotN, e.eta_ext, e.eta_int));

		const float factor = cosThetaT < 0.0f ? e.eta_ext / e.eta_int : e.eta_int / e.eta_ext;
		result *= factor * factor;
	}
#ifndef BXDFS_EVAL_NO_COS
	if (reflection) {
		result *= max(woDotN, 0.0f);
	} else {
		result *= max(-woDotN, 0.0f);
	}
#endif

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (any(isnan(result))) debugPrintfEXT("NAN: dielectric_eval result");
#endif
	if (any(isnan(result))) return vec3(0.0f);
	return result;
}

float /*pdf*/ dielectric_pdf(BsdfScatterEvent e, const vec3 wo) {
	const bool rZero = (e.roughness == 0.0f);
	if(rZero) return 0.0f;

	
	if(dot(e.wi, e.geo_n) < 0) {
		e.n = -e.n;
		e.geo_n = -e.geo_n;
		float temp = e.eta_ext;
		e.eta_ext = e.eta_int;
		e.eta_int = temp;
	}
	const float wiDotN = dot(e.wi, e.n);

	const float woDotN = dot(wo, e.n);
	const bool reflection = (wiDotN * woDotN) >= 0.0f;

	vec3 m;
	if (reflection) m = halfvector(e.wi, wo);
	else m = halfvector(e.wi, wo, e.eta_ext, e.eta_int);
	
	m *= sign(dot(m, e.n));

	const float mDotN = dot(m, e.n);
	const float wiDotM = dot(e.wi, m);
	const float woDotM = dot(wo, m);

	if (mDotN <= 0.0f) return 0;
	if (reflection && dot(wo, e.geo_n) <= 0) return 0;
	if (!reflection && dot(wo, e.geo_n) >= 0) return 0;
	

	float cosThetaT;
	const float fi = F_Dielectric(e.eta_ext, e.eta_int, abs(wiDotM), cosThetaT);
	float pdf = ggx_pdf(e.n, m, e.wi, e.roughness);

	if (reflection) pdf *= fi * D_PDF_M_TO_WO(woDotM);
	else pdf *= (1.0f - fi) * D_PDF_M_TO_WO(wiDotM, woDotM, e.eta_ext, e.eta_int);

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (isnan(pdf)) debugPrintfEXT("NAN: dielectric_pdf pdf");
#endif
	if (isnan(pdf)) return 0.0f;
	return pdf;
}

bool dielectric_sample(BsdfScatterEvent e, const vec3 rand, REF(vec3) wo, REF(float) pdf, REF(vec3) weight){

	
	if(dot(e.wi, e.geo_n) < 0) {
		e.n = -e.n;
		e.geo_n = -e.geo_n;
		float temp = e.eta_ext;
		e.eta_ext = e.eta_int;
		e.eta_int = temp;
	}
	const float wiDotN = dot(e.wi, e.n);

	const float sampleRoughness = (1.2f - 0.2f*sqrt(abs(wiDotN))) * e.roughness;
	
	const bool rZero = (e.roughness == 0.0f);
	vec3 m =  rZero ? e.n : ggx_sample(e.wi, e.n, sampleRoughness, rand.xy);
	
	m *= sign(dot(m, e.geo_n));
	const float wiDotM = dot(e.wi, m);

	float cosThetaT;
	const float fi = F_Dielectric(e.eta_ext, e.eta_int, abs(wiDotM), cosThetaT);
	const bool reflection = rand.z < fi;

	if (reflection) /* specular */ {
		wo = reflectionDir(e.wi, m);
	} else /* refract */ {
		wo = refractionDir(e.wi, m, e.eta_ext, e.eta_int, cosThetaT);
		
		if (all(equal(wo, vec3(0)))) return false;
		if (e.eta_ext == e.eta_int) wo = -e.wi;
	}

	const float woDotN = dot(wo, e.n);
	const float woDotM = dot(wo, m);
	const float mDotN = dot(m, e.n);

	if (mDotN <= 0.0f) return false;
	if (reflection && dot(wo, e.geo_n) <= 0) return false;
	if (!reflection && dot(wo, e.geo_n) >= 0) return false;
	

	pdf = !rZero ? ggx_pdf(e.n, m, e.wi, e.roughness) : 0.0f;
	weight = e.albedo * vec3(!rZero ? ggx_weight(wiDotM, woDotM, wiDotN, woDotN, mDotN, e.roughness) : 1.0f);
	if (reflection) /* specular */ {
		if(!rZero) pdf *= D_PDF_M_TO_WO(woDotM);
		
		
		pdf *= fi;
	}
	else /* refract */ {
		if(!rZero) pdf *= D_PDF_M_TO_WO(wiDotM, woDotM, e.eta_ext, e.eta_int);
		
		
		pdf *= (1.0f - fi);
		
		const float factor = cosThetaT < 0.0f ? e.eta_ext / e.eta_int : e.eta_int / e.eta_ext;
		weight *= factor * factor;
	}

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (isnan(pdf)) debugPrintfEXT("NAN: dielectric_sample pdf");
	if (any(isnan(weight))) debugPrintfEXT("NAN: dielectric_sample weight");
#endif
	if (isnan(pdf) || any(isnan(weight))) return false;
	return true;
}

/*********************/
/*  THIN/DIELECTRIC  */
/*********************/

vec3 /*brdf*/ thindielectric_eval(BsdfScatterEvent e, const vec3 wo) { return vec3(0.0f); }

float /*pdf*/ thindielectric_pdf(BsdfScatterEvent e, const vec3 wo) { return 0.0f; }

bool thindielectric_sample(BsdfScatterEvent e, const vec3 rand, REF(vec3) wo, REF(float) pdf, REF(vec3) weight) {
	
	float eta_t = e.eta_int;
	float eta_i = e.eta_ext;
	
	if(dot(e.wi, e.geo_n) < 0) {
		e.n = -e.n;
		e.geo_n = -e.geo_n;
	}
	const float wiDotN = dot(e.wi, e.n);

	const float sampleRoughness = (1.2f - 0.2f * sqrt(abs(wiDotN))) * e.roughness;
	const bool rZero = (e.roughness == 0.0f);
	const vec3 m = rZero ? e.n : ggx_sample(e.wi, e.n, sampleRoughness, rand.xy);
	const float wiDotM = dot(e.wi, m);

	float cosThetaT;
	float R = F_Dielectric(eta_i, eta_t, abs(wiDotM), cosThetaT);
    const float T = 1.0f - R;

    
    if (R < 1.0f) R += T * T * R / (1.0f - R * R);

    const bool reflection = rand.z < R;

	if (reflection) /* specular */ {
        wo = reflectionDir(e.wi, m);
	}
	else /* refract */ { 
		wo = -e.wi;
	}

	const float woDotN = dot(wo, e.n);
	const float woDotM = dot(wo, m);
	const float mDotN = dot(m, e.n);
	
	if (mDotN <= 0.0f) return false;
	if (reflection && dot(wo, e.geo_n) <= 0) return false;
	if (!reflection && dot(wo, e.geo_n) >= 0) return false;
	

	pdf = !rZero ? ggx_pdf(e.n, m, e.wi, e.roughness) : 0.0f;
	
	weight = e.albedo * vec3(!rZero ? ggx_weight(wiDotM, woDotM, wiDotN, woDotN, mDotN, e.roughness) : 1.0f);
	if (reflection) /* specular */ {
		if(!rZero) pdf *= D_PDF_M_TO_WO(woDotM);
		
		
		pdf *= R;
	}
	else /* refract */ {
		if(!rZero) pdf *= D_PDF_M_TO_WO(wiDotM, woDotM, eta_i, eta_t);
		
		
		pdf *= 1.0f - R;
	}

#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (isnan(pdf)) debugPrintfEXT("NAN: thindielectric_sample pdf");
	if (any(isnan(weight))) debugPrintfEXT("NAN: thindielectric_sample weight");
#endif
	if (isnan(pdf) || any(isnan(weight))) return false;
	return true;
}

/**********************/
/*        MIX         */
/**********************/

vec3 mix_eval(const vec3 brdf0, const vec3 brdf1, const float ratio){
	return mix(brdf0, brdf1, ratio);
}
float mix_pdf(const float pdf0, const float pdf1, const float ratio){
	return mix(pdf0, pdf1, ratio);
}
uint mix_sample(const float ratio, const float rand){
	/* select brdf 1 */ if(rand < ratio) return 1;
	/* select brdf 0 */ else return 0;
}
vec4 mix_sample(const vec4 dir_pdf0, const vec4 dir_pdf1, const float ratio, const float rand){
	/* select brdf 1 */ if(rand < ratio) return dir_pdf1;
	/* select brdf 0 */ else return dir_pdf0;
}

/**********************/
/* METALLIC/ROUGHNESS */
/**********************/

vec3 /*brdf*/ metalroughness_eval(BsdfScatterEvent e, const vec3 wo) {
	const vec3 pl_brdf = plastic_eval(e, wo);
	const vec3 di_brdf = e.thin ? thindielectric_eval(e, wo) : dielectric_eval(e, wo);							
	const vec3 co_brdf = conductor_eval(e, wo);
	return mix(mix(pl_brdf, di_brdf, e.transmission), co_brdf, e.metallic);
}

float /*pdf*/ metalroughness_pdf(BsdfScatterEvent e, const vec3 wo) {
	const float pl_pdf = plastic_pdf(e, wo);
	const float di_pdf = e.thin ? thindielectric_pdf(e, wo) : dielectric_pdf(e, wo);
	const float co_pdf = conductor_pdf(e, wo);
	return mix(mix(pl_pdf, di_pdf, e.transmission), co_pdf, e.metallic);
}

bool metalroughness_sample(BsdfScatterEvent e, const vec4 rand0, const vec3 rand1, REF(vec3) wo, REF(float) pdf, REF(vec3) weight) {
	if(rand0.x < e.metallic) {
		return conductor_sample(e, rand1.xy, wo, pdf, weight);
	} else {
		if (rand0.y < e.transmission) {
			if(e.thin) return thindielectric_sample(e, rand1, wo, pdf, weight);
			else return dielectric_sample(e, rand1, wo, pdf, weight);
		} else {
			return plastic_sample(e, rand1, rand0.zw, wo, pdf, weight);
		}
	}
}

/***********************/
/* SPECULAR/GLOSSINESS */
/***********************/




#endif /* GLSL_BSDF */