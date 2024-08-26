#ifndef GLSL_IALT_GGX
#define GLSL_IALT_GGX
#include "../utils/glsl/bxdfs/microfacet.glsl"
#include "../utils/glsl/bxdfs/fresnel.glsl"

bool plastic_sample_test(const vec3 wi, const vec3 n, const vec3 albedo, const float roughness, const float eta_i, const float eta_o, 
					const vec3 rand0, const vec2 rand1, out vec3 wo, out float pdf, out vec3 weight){
	const float wiDotN = dot(wi,n);
	const float sampleRoughness = roughness;
	const float sampleAlpha = sampleRoughness * sampleRoughness;
	const vec3 m = ggx_sample(wi, n, sampleRoughness, rand0.xy);
	const float wiDotM = dot(wi,m);

	const float eta_i_m = eta_i;
	const float eta_o_m = eta_o;

	float cosThetaT;
	const float fi = F_Dielectric(eta_i, eta_o, wiDotN, cosThetaT);
	const bool reflection = rand0.z < fi;

	if(reflection) /* specular */ {
		wo = reflectionDir(wi, m);
	} else /* lambert */ {
		wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereCosine(rand1.xy), n));	
	}

    const float woDotN = dot(wo,n);
    const float woDotM = dot(wo,m);
	const float mDotN = dot(m,n);
	
	
	
	if (mDotN <= 0.0f) return false;
	
	
	
	if (all(equal(wi+wo, vec3(0.0f)))) return false;

	pdf = ggx_pdf(n, m, wi, sampleRoughness);
	
	weight = vec3(ggx_weight(wiDotM, woDotM, wiDotN, woDotN, mDotN, sampleAlpha));
	if(reflection) /* specular */ {
		pdf *= D_PDF_M_TO_WO(woDotM);
		
		pdf *= fi;

		
	} else /* lambert */ {
        pdf = max(woDotN, 0.0f) * M_INV_PI;
        
        
        pdf *= (1.0f-fi);
        weight = albedo;
		
	}

	
	if(isnan(pdf)) return false;

	#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if(isnan(pdf)) debugPrintfEXT("NAN: dielectric_sample pdf");
	if(any(isnan(weight))) debugPrintfEXT("NAN: dielectric_sample weight");
	#endif
	return true;
}

bool plastic_sample(const vec3 wi, const vec3 n, const vec3 albedo, const float roughness, const float eta_i, const float eta_o, 
					const vec3 rand0, const vec2 rand1, out vec3 wo, out float pdf, out vec3 weight){
	const float wiDotN = dot(wi,n);
	const float sampleRoughness = roughness;
	const float sampleAlpha = sampleRoughness * sampleRoughness;
	const vec3 m = ggx_sample(wi, n, sampleRoughness, rand0.xy);
	const float wiDotM = dot(wi,m);

	const float eta_i_m = eta_i;
	const float eta_o_m = eta_o;

	float cosThetaT;
	const float fi = F_Dielectric(eta_i, eta_o, wiDotN, cosThetaT);
	const bool reflection = rand0.z < fi;

	if(reflection) /* specular */ {
		wo = reflectionDir(wi, m);
	} else /* lambert */ {
		wo = normalize(tangentSpaceToWorldSpace(sampleUnitHemisphereCosine(rand1.xy), n));	
	}

    const float woDotN = dot(wo,n);
    const float woDotM = dot(wo,m);
	const float mDotN = dot(m,n);
	
	
	
	if (mDotN <= 0.0f) return false;
	
	
	
	if (all(equal(wi+wo, vec3(0.0f)))) return false;

	
	
	
	if(reflection) /* specular */ {
		pdf = ggx_pdf(n, m, wi, sampleRoughness) * D_PDF_M_TO_WO(woDotM);
        weight = vec3(( ggx_eval(wi,wo,n,m,roughness) * BSDF_GGX(wiDotN, woDotN)));
		
		pdf *= fi;

		
	} else /* lambert */ {
        pdf = max(woDotN, 0.0f) * M_INV_PI;
        
        weight = (albedo * M_INV_PI);
        pdf *= (1.0f-fi);
        
		
	}

	
	if(isnan(pdf)) return false;

	#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if(isnan(pdf)) debugPrintfEXT("NAN: dielectric_sample pdf");
	if(any(isnan(weight))) debugPrintfEXT("NAN: dielectric_sample weight");
	#endif
	return true;
}

#endif /* GLSL_IALT_GGX */
