

#ifndef GLSL_BSDF_VOLUME
#define GLSL_BSDF_VOLUME

#ifndef M_2PI
#define M_2PI 6.28318530717959f
#endif
#ifndef M_INV_2PI
#define M_INV_2PI 0.159154943091895f
#endif
#ifndef M_INV_4PI
#define M_INV_4PI 0.0795774715459477f
#endif

#ifndef GLSL_RENDERING_UTILS
#include "../rendering_utils.glsl"
#endif








float transmittance(const float sigma_t, const float dist) {
    return exp(-sigma_t * dist);
}
float sampleDistance(const float sigma_t, const float rand) {
    return -log(1.0f - rand) / sigma_t;
}
float sampleDistancePdf(const float sigma_t, const float dist, const float rand) {
    return sigma_t * transmittance(sigma_t, dist);
}








float hgPhaseFunction(const float cosTheta, const float g)
{
    const float denom = 1.0f + g * g - 2.0f * g * cosTheta;
    return M_INV_4PI * (1.0f - g * g) / (denom * sqrt(denom));
}

#define hgPdf (M_INV_2PI)


vec2 hgSample(vec3 w, float g, const vec2 rand)
{
    float cosTheta;
    if (abs(g) < 0.001f) cosTheta = 1.0f - 2.0f * rand[0];
    else {
        const float sqrTerm = (1.0f - g * g) / (1.0f - g + 2.0f * g * rand[0]);
        cosTheta = (1.0f + g * g - sqrTerm * sqrTerm) / (2.0f * g);
    }

    const float theta = acos(cosTheta);
    const float phi = M_2PI * rand[1];
    return vec2(theta, phi);
    
    
    

    
    

    
}


#endif /* GLSL_BSDF_VOLUME */