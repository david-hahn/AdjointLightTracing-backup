
vec3 evalSHSumRGBTarget(const int order, const int dataOffset, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            colour += evalSH(l, m, phi, theta) * vec3(
                TARGET_SH_DATA_BUFFER_[dataOffset + idxX3],
                TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 1],
                TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 2]
            );
        }
    }
    return colour;
}

vec3 evalSHSumRGBTarget(const int order, const int dataOffset, const vec3 dir) { 
    float phi, theta;
    toSphericalCoords(dir, phi, theta); 
    return evalSHSumRGBTarget(order, dataOffset, phi, theta);
}

vec3 evalInterSHSumRGBTarget(const int order, const uvec3 dataOffsets, const vec3 bary, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            colour += evalSH(l, m, phi, theta) * vec3(
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 0] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 0] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 0],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]
            );
        }
    }
    return colour;
}
vec3 evalInterSHSumRGBTarget(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir) { 
    float phi, theta;
    toSphericalCoords(dir, phi, theta); 
    return evalInterSHSumRGBTarget(order, dataOffsets, bary, phi, theta);
}

vec3 evalSHSumRGBAdjoint(const int order, const int dataOffset, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            const vec3 x = vec3(SH_DATA_BUFFER_[dataOffset + idxX3], SH_DATA_BUFFER_[dataOffset + idxX3 + 1], SH_DATA_BUFFER_[dataOffset + idxX3 + 2]);
            const vec3 target = vec3(TARGET_SH_DATA_BUFFER_[dataOffset + idxX3], TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 1], TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 2]);
            const vec3 xMinusTarget = x - target;
            
            colour += evalSH(l, m, phi, theta) * (0.5f * xMinusTarget * xMinusTarget);
        }
    }
    return colour;
}

vec3 evalSHSumRGBAdjoint(const int order, const int dataOffset, const vec3 dir) { 
    float phi, theta;
    toSphericalCoords(dir, phi, theta); 
    return evalSHSumRGBAdjoint(order, dataOffset, phi, theta);
}

vec3 evalInterSHSumRGBAdjoint(const int order, const uvec3 dataOffsets, const vec3 bary, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            const vec3 x = vec3(bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 0] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 0] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 0],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]);
            const vec3 target = vec3(bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 0] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 0] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 0],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]);
            const vec3 xMinusTarget = x - target;
            colour += evalSH(l, m, phi, theta) * (0.5f * xMinusTarget * xMinusTarget);
        }
    }
    return colour;
}
vec3 evalInterSHSumRGBAdjoint(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir) { 
    float phi, theta;
    toSphericalCoords(dir, phi, theta); 
    return evalInterSHSumRGBAdjoint(order, dataOffsets, bary, phi, theta);
}

vec3 evalHSHSumRGBTarget(const int order, const int dataOffset, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            colour += evalHSH(l, m, phi, theta) * vec3(
                TARGET_SH_DATA_BUFFER_[dataOffset + idxX3],
                TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 1],
                TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 2]
            );
        }
    }
    return colour;
}

vec3 evalHSHSumRGBTarget(const int order, const int dataOffset, const vec3 dir, const vec3 normal, const vec3 tangent) { 
    float phi, theta;
    toHemiSphericalCoords(dir, normal, tangent, phi, theta); 
    return evalHSHSumRGBTarget(order, dataOffset, phi, theta);
}

vec3 evalInterHSHSumRGBTarget(const int order, const uvec3 dataOffsets, const vec3 bary, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            colour += evalHSH(l, m, phi, theta) * vec3(
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 0] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 0] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 0],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]
            );
        }
    }
    return colour;
}
vec3 evalInterHSHSumRGBTarget(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir, const vec3 normal, const vec3 tangent) { 
    float phi, theta;
    toHemiSphericalCoords(dir, normal, tangent, phi, theta); 
    return evalInterHSHSumRGBTarget(order, dataOffsets, bary, phi, theta);
}

vec3 evalHSHSumRGBAdjoint(const int order, const int dataOffset, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            const vec3 x = vec3(SH_DATA_BUFFER_[dataOffset + idxX3], SH_DATA_BUFFER_[dataOffset + idxX3 + 1], SH_DATA_BUFFER_[dataOffset + idxX3 + 2]);
            const vec3 target = vec3(TARGET_SH_DATA_BUFFER_[dataOffset + idxX3], TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 1], TARGET_SH_DATA_BUFFER_[dataOffset + idxX3 + 2]);
            const vec3 xMinusTarget = x - target;         
            colour += evalHSH(l, m, phi, theta) * (0.5f * xMinusTarget * xMinusTarget);
        }
    }
    return colour;
}

vec3 evalHSHSumRGBAdjoint(const int order, const int dataOffset, const vec3 dir, const vec3 normal, const vec3 tangent) { 
    float phi, theta;
    toHemiSphericalCoords(dir, normal, tangent, phi, theta); 
    return evalHSHSumRGBAdjoint(order, dataOffset, phi, theta);
}

vec3 evalInterHSHSumRGBAdjoint(const int order, const uvec3 dataOffsets, const vec3 bary, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            const vec3 x = vec3(bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 0] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 0] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 0],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]);
            const vec3 target = vec3(bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 0] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 0] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 0],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * TARGET_SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * TARGET_SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * TARGET_SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]);
            const vec3 xMinusTarget = x - target;
            colour += evalHSH(l, m, phi, theta) * (0.5f * xMinusTarget * xMinusTarget);
        }
    }
    return colour;
}
vec3 evalInterHSHSumRGBAdjoint(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir, const vec3 normal, const vec3 tangent) { 
    float phi, theta;
    toHemiSphericalCoords(dir, normal, tangent, phi, theta); 
    return evalInterHSHSumRGBAdjoint(order, dataOffsets, bary, phi, theta);
}
