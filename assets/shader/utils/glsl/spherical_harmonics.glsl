#ifndef GLSL_SPHERICAL_HARMONICS
#define GLSL_SPHERICAL_HARMONICS
#extension GL_EXT_control_flow_attributes : enable





















float factorial(const int x) {
    switch(x) {
        case 0: return 1.0f;
        case 1: return 1.0f;
        case 2: return 2.0f;
        case 3: return 6.0f;
        case 4: return 24.0f;
        case 5: return 120.0f;
        case 6: return 720.0f;
        case 7: return 5040.0f;
        case 8: return 40320.0f;
        case 9: return 362880.0f;
        case 10: return 3628800.0f;
        case 11: return 39916800.0f;
        case 12: return 479001600.0f;
        case 13: return 6227020800.0f;
        case 14: return 87178291200.0f;
        case 15: return 1307674368000.0f;
        case 16: return 20922789888000.0f;
        case 17: return 355687428096000.0f;
        default: break;
    }
    float s = 1.0f;
    [[unroll, dependency_infinite]]
    for (int n = 1; n <= x; n++) {
        s *= float(n);
    }
    return s;
}

float doubleFactorial(const int x) {
    switch(x) {
        case 0: return 1.0f;
        case 1: return 1.0f;
        case 2: return 2.0f;
        case 3: return 3.0f;
        case 4: return 8.0f;
        case 5: return 15.0f;
        case 6: return 48.0f;
        case 7: return 105.0f;
        case 8: return 384.0f;
        case 9: return 945.0f;
        case 10: return 3840.0f;
        case 11: return 10395.0f;
        case 12: return 46080.0f;
        case 13: return 135135.0f;
        case 14: return 645120.0f;
        case 15: return 2027025.0f;
        case 16: return 10321920.0f;
        case 17: return 34459425.0f;
        default: break;
    }
    float s = 1.0f;
    float n = float(x);
    [[unroll, dependency_infinite]]
    while (n >= 2.0) {
        s *= n;
        n -= 2.0;
    }
    return s;
}











float evalLegendrePolynomial(const int l, const int m, const float x) {
    
    
    float pmm = 1.0;
    
    if (m > 0) {
        float sign = (m % 2 == 0 ? 1.0f : -1.0f);
        pmm = sign * doubleFactorial(2 * m - 1) * pow(1.0f - x * x, m / 2.0f);
    }

    if (l == m) {
        
        return pmm;
    }

    
    float pmm1 = x * (2.0f * float(m) + 1.0f) * pmm;
    if (l == m + 1) {
        
        return pmm1;
    }

    
    
    
    [[unroll, dependency_infinite]]
    for (int n = m + 2; n <= l; n++) {
        const float pmn = (x * (2 * n - 1) * pmm1 - (n + m - 1) * pmm) / (n - m);
        pmm = pmm1;
        pmm1 = pmn;
    }
    
    return pmm1;
}





#define EVAL_SH(norm, costheta)                                                                                         \
    /*CHECK(l >= 0, "l must be at least 0.");*/                                                                         \
    /*CHECK(-l <= m && m <= l, "m must be between -l and l.");*/                                                        \
    const int absM = abs(m);                                                                                            \
    const float kml = sqrt((2.0f * l + 1.0f) * factorial(l - absM) / ((norm) * factorial(l + absM)));                   \
    const float kmlTimesLPolyEval = kml * evalLegendrePolynomial(l, absM, (costheta));                                  \
    if (m == 0) return kmlTimesLPolyEval;                                                                               \
    const float mTimesPhi = absM * phi;                                                                                 \
    const float sh = 1.414213562373095f /*sqrt(2)*/ * kmlTimesLPolyEval;                                                \
    if (m > 0) return sh * cos(mTimesPhi);                                                                              \
    else return sh * sin(mTimesPhi);



float smoothSH(const int l, const float factor) {
    return 1.0f / (1.0f + factor * float((l * (l + 1)) * (l * (l + 1))));
}


uint getIndexSH(const int l, const int m) {
    return uint(l * (l + 1) + m);
}
void dirToSphericalCoordsSH(in vec3 dir, out float theta, out float phi) { 
	theta = atan(sqrt(dir.x * dir.x + dir.y * dir.y), dir.z);
    phi = atan(dir.y, dir.x);
}


float evalOrthonormSH(const int l, const int m, const float theta, const float phi) {
    EVAL_SH(12.5663706144f /*4PI*/, cos(theta))
}

float evalOrthonormSH(const int l, const int m, const vec3 dir) {
    float theta, phi;
    dirToSphericalCoordsSH(dir, theta, phi);
    return evalOrthonormSH(l, m, theta, phi);
}


float eval4PiNormSH(const int l, const int m, const float theta, const float phi) {
    EVAL_SH(1.0f, cos(theta))
}

float eval4PiNormSH(const int l, const int m, const vec3 dir) {
    float theta, phi;
    dirToSphericalCoordsSH(dir, theta, phi);
    return eval4PiNormSH(l, m, theta, phi);
}


#define getIndexHSH getIndexSH

void dirToSphericalCoordsHSH(in vec3 dir, in vec3 normal, in vec3 tangent, out float theta, out float phi) { 
    const vec3 bitangent = cross(normal, tangent);
	
    theta = acos(dot(dir, normal));
    phi = atan(dot(dir, bitangent), dot(dir, tangent));
}


float evalOrthonormHSH(const int l, const int m, const float theta, const float phi) {
    EVAL_SH(6.28318530718 /*2Pi*/, 2.0f * cos(theta) - 1.0f)
}

float evalOrthonormHSH(const int l, const int m, const vec3 dir, const vec3 normal, const vec3 tangent, const bool clampTheta) { 
    float theta, phi;
    dirToSphericalCoordsHSH(dir, normal, tangent, theta, phi);
    
    if(clampTheta) theta = clamp(theta, 0.0, 1.570796326794897f /*pi/2*/);
    
    else if(dot(dir, normal) < 0.0) return 0.0;

    return evalOrthonormHSH(l, m, theta, phi);
}


float eval2PiNormHSH(const int l, const int m, const float theta, const float phi) {
    EVAL_SH(1.0f, 2.0f * cos(theta) - 1.0f)
}

float eval2PiNormHSH(const int l, const int m, const vec3 dir, const vec3 normal, const vec3 tangent, const bool clampTheta) { 
    float theta, phi;
    dirToSphericalCoordsHSH(dir, normal, tangent, theta, phi);
    
    if(clampTheta) theta = clamp(theta, 0.0, 1.570796326794897f /*pi/2*/);
    
    else if(dot(dir, normal) < 0.0) return 0.0;

    return eval2PiNormHSH(l, m, theta, phi);
}

#undef EVAL_SH
#endif /* GLSL_SPHERICAL_HARMONICS */