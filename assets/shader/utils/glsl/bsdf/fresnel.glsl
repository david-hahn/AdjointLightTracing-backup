

#ifndef GLSL_BSDF_FRESNEL
#define GLSL_BSDF_FRESNEL

#ifndef __cplusplus
    #ifndef REF
        #define REF(type) inout type
    #endif
    #ifndef OREF
        #define OREF(type) out type
    #endif
#endif


#ifndef RI_VACUUM
#define RI_VACUUM 		1.0f
#define RI_AIR 			1.000293f
#define RI_WATER 		1.333f
#define RI_ICE 			1.31f
#define RI_GLASS 		1.52f
#define RI_FLINT_GLASS 	1.69f
#define RI_SAPPHIRE 	1.77f
#define RI_DIAMOND 	    2.42f
#endif

float F0(const float refractionIndex1, const float refractionIndex2){
    float F0 = (refractionIndex1 - refractionIndex2) / (refractionIndex1 + refractionIndex2);
    return F0 * F0;
}
vec3 F0(const vec3 refractionIndex1, const vec3 refractionIndex2){
    vec3 F0 = (refractionIndex1 - refractionIndex2) / (refractionIndex1 + refractionIndex2);
    return F0 * F0;
}










float F_Schlick(const float cosTheta)
{
    const float f = clamp(1.0f - cosTheta, 0.0f, 1.0f);
    const float f2 = f * f;
    return f2 * f2 * f;
}
float F_Schlick(const float cosTheta, const float F0, const float F90)
{   
    return F0 + (F90 - F0) * F_Schlick(cosTheta); 
}
vec3 F_Schlick(const float cosTheta, const vec3 F0, const float F90)
{   
    return F0 + (vec3(F90) - F0) * F_Schlick(cosTheta); 
}
float F_Schlick(const float cosTheta, const float F0)
{   
    return F_Schlick(cosTheta, F0, 1.0f);  
}
vec3 F_Schlick(const float cosTheta, const vec3 F0)
{   
    return F_Schlick(cosTheta, F0, 1.0f); 
}

/*
*   https:
*   https:
*   https:
*   eta_i: index of refraction for medium outside object
*   eta_t: index of refraction for medium inside object
*   cosThetaI: angle between incoming ray and normal
*   cosThetaT: angle between refracted ray and negative normal
*   return: reflection amount 
*/
float F_Dielectric(REF(float) eta_i, REF(float) eta_t, float cosThetaI, OREF(float) cosThetaT) {
    
    
    if(cosThetaI <= 0){
        const float buff = eta_i;
        eta_i = eta_t;
        eta_t = buff;
        cosThetaI = abs(cosThetaI);
    }
    const float eta = eta_i/eta_t;
    
    if (eta == 1.0f) {
        cosThetaT = -cosThetaI;
        return 0.0f;
    }
    /* Using Snell's law, calculate the squared sine of the 
       angle between the normal and the transmitted ray */
    const float sinThetaTSq = eta * eta * (1.0f - cosThetaI * cosThetaI);

    
    if (sinThetaTSq >= 1.0f) {
        cosThetaT = 0.0f;
        return 1.0f;
    }

    cosThetaT = sqrt(max(1.0f - sinThetaTSq, 0.0f));

    const float rs =  (eta_i * cosThetaI - eta_t * cosThetaT)
                    / (eta_i * cosThetaI + eta_t * cosThetaT); /*⊥*/
    const float rp =  (eta_t * cosThetaI - eta_i * cosThetaT)
                    / (eta_t * cosThetaI + eta_i * cosThetaT); /*∥*/

    /* No polarization -- return the unpolarized reflectance */
    return 0.5f * (rs * rs + rp * rp);
}

/*
*   https:
*   eta:
*   k: extinction coefficient
*   cosThetaI: angle between incoming ray and normal
*   return: reflection amount 
*/


vec3 F_Conductor(const vec3 eta, const vec3 k, const float cosThetaI)
{
    const float cosThetaI2 = cosThetaI * cosThetaI;
    const float sinThetaI2 = max(1.0f - cosThetaI2, 0.0f);
    const float sinThetaI4 = sinThetaI2 * sinThetaI2;

    const vec3 eta2 = eta * eta;
    const vec3 k2 = k * k;
    const vec3 innerTerm = eta2 - k2 - sinThetaI2;
    const vec3 a2PlusB2 = sqrt(max(innerTerm * innerTerm + 4.0f * eta2 * k2, 0.0f));
    const vec3 a = sqrt(max((a2PlusB2 + innerTerm) * 0.5f, 0.0f));

    const vec3 A = a2PlusB2 + cosThetaI2;
    const vec3 B = 2.0f * cosThetaI * a;
    const vec3 C = A + sinThetaI4;
    const vec3 D = B * sinThetaI2;

    const vec3 rs = (A - B)
                   / (A + B); /*⊥*/
    const vec3 rp = rs * (C - D)
                   / (C + D); /*∥*/

    return 0.5f * (rs + rp);
}

/*
** reflection
*/




vec3 reflectionDir(const vec3 wi, const vec3 n){
    return 2.0f * dot(wi, n) * n - wi;
}
/* Bram de Greve 2006 - Reflections and Refractions in Ray Tracing */











/*
** refraction
*/









vec3 refractionDir(const vec3 wi, const vec3 n, const float eta_i, const float eta_t, const float cosThetaT)
{
    const float eta = eta_i / eta_t;
    const float wiDotN = dot(wi, n);
    return (eta * wiDotN - sign(wiDotN) * cosThetaT) * n - eta * wi;
}
vec3 refractionDir(const vec3 wi, const vec3 n, const float eta_i, const float eta_t)
{
    const float eta = eta_i / eta_t;
    const float wiDotN = dot(wi, n);
    const float wiDotN2 = wiDotN * wiDotN;
    const float sinThetaTSq = eta * eta * (1.0f - wiDotN2);
    const float cosThetaT = sqrt(max(1 - sinThetaTSq, 0.0f));
    return (eta * wiDotN - sign(wiDotN) * cosThetaT) * n - eta * wi;
}




float fresnelDiffuseReflectance(const float eta_ext, const float eta_int) {
    const float eta = eta_int / eta_ext;
    const float invEta = eta_ext / eta_int;
    /* Fast mode: the following code approximates the
    ** diffuse Frensel reflectance for the eta<1 and
    ** eta>1 cases. An evalution of the accuracy led
    ** to the following scheme, which cherry-picks
    ** fits from two papers where they are best.
    */
    if (eta < 1) {
        /* Fit by Egan and Hilgeman (1973). Works
        ** reasonably well for "normal" IOR values (<2).

        ** Max rel. error in 1.0 - 1.5 : 0.1%
        ** Max rel. error in 1.5 - 2   : 0.6%
        ** Max rel. error in 2.0 - 5   : 9.5%
        */
        return -1.4399f * (eta * eta)
                + 0.7099f * eta
                + 0.6681f
                + 0.0636f / eta;
    } else {
        /* Fit by d'Eon and Irving (2011)
        **
        ** Maintains a good accuracy even for
        ** unrealistic IOR values.
        **
        ** Max rel. error in 1.0 - 2.0   : 0.1%
        ** Max rel. error in 2.0 - 10.0  : 0.2%
        */
        const float invEta2 = invEta*invEta;
        const float invEta3 = invEta2*invEta;
        const float invEta4 = invEta3*invEta;
        const float invEta5 = invEta4*invEta;

        return 0.919317f - 3.4793f * invEta
                + 6.75335f * invEta2
                - 7.80989f * invEta3
                + 4.98554f * invEta4
                - 1.36881f * invEta5;
    }

    return 0.0f;
}

/* 
** UNTESTED/UNFINISHED 
*/

















#endif /* GLSL_BSDF_FRESNEL */