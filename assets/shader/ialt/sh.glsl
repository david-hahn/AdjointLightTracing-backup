



#define JOIN(x, y) JOIN_AGAIN(x, y)
#define JOIN_AGAIN(x, y) x ## y

#define FUNC_NAME_EVAL_RGB_SH JOIN(SH_DATA_BUFFER, EvalRgbSH)
#define FUNC_NAME_EVAL_INTER_RGB_SH JOIN(SH_DATA_BUFFER, EvalInterRgbSH)
#define FUNC_NAME_EVAL_RGB_HSH JOIN(SH_DATA_BUFFER, EvalRgbHSH)
#define FUNC_NAME_EVAL_INTER_RGB_HSH JOIN(SH_DATA_BUFFER, EvalInterRgbHSH)


vec3 FUNC_NAME_EVAL_RGB_SH(const int order, const int dataOffset, const vec3 dir) {
    float theta, phi;
    dirToSphericalCoordsSH(dir, theta, theta);
    
    vec3 color = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getIndexSH(l, m);
            color += evalOrthonormSH(l, m, theta, phi) * vec3(
                SH_DATA_BUFFER[dataOffset + idxX3 + 0],
                SH_DATA_BUFFER[dataOffset + idxX3 + 1],
                SH_DATA_BUFFER[dataOffset + idxX3 + 2]
            );
        }
    }
    return color;
}


vec3 FUNC_NAME_EVAL_INTER_RGB_SH(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir) {
    float theta, phi;
    dirToSphericalCoordsSH(dir, theta, phi);
    
    vec3 color = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getIndexSH(l, m);
            color += evalOrthonormSH(l, m, theta, phi) * vec3(
                bary.x * SH_DATA_BUFFER[dataOffsets.x + idxX3] + bary.y * SH_DATA_BUFFER[dataOffsets.y + idxX3] + bary.z * SH_DATA_BUFFER[dataOffsets.z + idxX3],
                bary.x * SH_DATA_BUFFER[dataOffsets.x + idxX3 + 1] + bary.y * SH_DATA_BUFFER[dataOffsets.y + idxX3 + 1] + bary.z * SH_DATA_BUFFER[dataOffsets.z + idxX3 + 1],
                bary.x * SH_DATA_BUFFER[dataOffsets.x + idxX3 + 2] + bary.y * SH_DATA_BUFFER[dataOffsets.y + idxX3 + 2] + bary.z * SH_DATA_BUFFER[dataOffsets.z + idxX3 + 2]
            );
        }
    }
    return color;
}


vec3 FUNC_NAME_EVAL_RGB_HSH(const int order, const int dataOffset, const vec3 dir, 
        const vec3 normal, const vec3 tangent, const bool clampTheta) {
    float theta, phi;
    dirToSphericalCoordsHSH(dir, normal, tangent, theta, phi);
    
    if(clampTheta) theta = clamp(theta, 0.0, 1.570796326794897f /*pi/2*/);
    
    else if(dot(dir, normal) < 0.0) return vec3(0.0);
    
    vec3 color = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getIndexSH(l, m);
            color += evalOrthonormHSH(l, m, theta, phi) * vec3(
                SH_DATA_BUFFER[dataOffset + idxX3],
                SH_DATA_BUFFER[dataOffset + idxX3 + 1],
                SH_DATA_BUFFER[dataOffset + idxX3 + 2]
            );
        }
    }
    return color;
}

vec3 FUNC_NAME_EVAL_INTER_RGB_HSH(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir, 
        const vec3 normal, const vec3 tangent, const bool clampTheta) {
    float theta, phi;
    dirToSphericalCoordsHSH(dir, normal, tangent, theta, phi);
    
    if(clampTheta) theta = clamp(theta, 0.0, 1.570796326794897f /*pi/2*/);
    
    else if(dot(dir, normal) < 0.0) return vec3(0.0);
    
    vec3 color = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getIndexSH(l, m);
            color += evalOrthonormHSH(l, m, theta, phi) * vec3(
                bary.x * SH_DATA_BUFFER[dataOffsets.x + idxX3    ] + bary.y * SH_DATA_BUFFER[dataOffsets.y + idxX3    ] + bary.z * SH_DATA_BUFFER[dataOffsets.z + idxX3    ],
                bary.x * SH_DATA_BUFFER[dataOffsets.x + idxX3 + 1] + bary.y * SH_DATA_BUFFER[dataOffsets.y + idxX3 + 1] + bary.z * SH_DATA_BUFFER[dataOffsets.z + idxX3 + 1],
                bary.x * SH_DATA_BUFFER[dataOffsets.x + idxX3 + 2] + bary.y * SH_DATA_BUFFER[dataOffsets.y + idxX3 + 2] + bary.z * SH_DATA_BUFFER[dataOffsets.z + idxX3 + 2]
            );
        }
    }
    return color;
}





#undef FUNC_NAME_EVAL_RGB_SH
#undef FUNC_NAME_EVAL_INTER_RGB_SH

#undef JOIN_AGAIN
#undef JOIN