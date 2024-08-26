#extension GL_EXT_control_flow_attributes : enable





















#ifndef SH_GLSL
#define SH_GLSL

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288f
#endif

float HardcodedSH00(const vec3 d) {
  
  return 0.282095;
}

float HardcodedSH1n1(const vec3 d) {
  
  return -0.488603 * d.y;
}

float HardcodedSH10(const vec3 d) {
  
  return 0.488603 * d.z;
}

float HardcodedSH1p1(const vec3 d) {
  
  return -0.488603 * d.x;
}

float HardcodedSH2n2(const vec3 d) {
  
  return 1.092548 * d.x * d.y;
}

float HardcodedSH2n1(const vec3 d) {
  
  return -1.092548 * d.y * d.z;
}

float HardcodedSH20(const vec3 d) {
  
  return 0.315392 * (-d.x * d.x - d.y * d.y + 2.0 * d.z * d.z);
}

float HardcodedSH2p1(const vec3 d) {
  
  return -1.092548 * d.x * d.z;
}

float HardcodedSH2p2(const vec3 d) {
  
  return 0.546274 * (d.x * d.x - d.y * d.y);
}

float HardcodedSH3n3(const vec3 d) {
  
  return -0.590044 * d.y * (3.0 * d.x * d.x - d.y * d.y);
}

float HardcodedSH3n2(const vec3 d) {
  
  return 2.890611 * d.x * d.y * d.z;
}

float HardcodedSH3n1(const vec3 d) {
  
  return -0.457046 * d.y * (4.0 * d.z * d.z - d.x * d.x
                             - d.y * d.y);
}

float HardcodedSH30(const vec3 d) {
  
  return 0.373176 * d.z * (2.0 * d.z * d.z - 3.0 * d.x * d.x
                             - 3.0 * d.y * d.y);
}

float HardcodedSH3p1(const vec3 d) {
  
  return -0.457046 * d.x * (4.0 * d.z * d.z - d.x * d.x
                             - d.y * d.y);
}

float HardcodedSH3p2(const vec3 d) {
  
  return 1.445306 * d.z * (d.x * d.x - d.y * d.y);
}

float HardcodedSH3p3(const vec3 d) {
  
  return -0.590044 * d.x * (d.x * d.x - 3.0 * d.y * d.y);
}

float HardcodedSH4n4(const vec3 d) {
  
  return 2.503343 * d.x * d.y * (d.x * d.x - d.y * d.y);
}

float HardcodedSH4n3(const vec3 d) {
  
  return -1.770131 * d.y * d.z * (3.0 * d.x * d.x - d.y * d.y);
}

float HardcodedSH4n2(const vec3 d) {
  
  return 0.946175 * d.x * d.y * (7.0 * d.z * d.z - 1.0);
}

float HardcodedSH4n1(const vec3 d) {
  
  return -0.669047 * d.y * d.z * (7.0 * d.z * d.z - 3.0);
}

float HardcodedSH40(const vec3 d) {
  
  float z2 = d.z * d.z;
  return 0.105786 * (35.0 * z2 * z2 - 30.0 * z2 + 3.0);
}

float HardcodedSH4p1(const vec3 d) {
  
  return -0.669047 * d.x * d.z * (7.0 * d.z * d.z - 3.0);
}

float HardcodedSH4p2(const vec3 d) {
  
  return 0.473087 * (d.x * d.x - d.y * d.y)
      * (7.0 * d.z * d.z - 1.0);
}

float HardcodedSH4p3(const vec3 d) {
  
  return -1.770131 * d.x * d.z * (d.x * d.x - 3.0 * d.y * d.y);
}

float HardcodedSH4p4(const vec3 d) {
  
  const float x2 = d.x * d.x;
  const float y2 = d.y * d.y;
  return 0.625836 * (x2 * (x2 - 3.0 * y2) - y2 * (3.0 * x2 - y2));
}

float HardcodedSH5n5(const vec3 d) {
    const float x2 = d.x * d.x;
    const float y2 = d.y * d.y;
    const float x4 = x2 * x2;
    const float y4 = y2 * y2;
    return -0.6564 * d.y * (5.0 * x4 - 10.0 * x2 * y2 + y4);
}

float HardcodedSH5n4(const vec3 d) {
    const float x2 = d.x * d.x;
    const float y2 = d.y * d.y;
    return 8.3026 * d.x * d.y * d.z * (x2 - y2);
}

float HardcodedSH5n3(const vec3 d) {
    const float x2 = d.x * d.x;
    const float y3 = d.y * d.y * d.y;
    const float z2 = d.z * d.z;
    return -0.0093 * (3.0 * x2 * d.y - y3) * (472.5000 * z2 - 52.5000);
}

float HardcodedSH5n2(const vec3 d) {
    const float z2 = d.z * d.z;
    return 4.7935 * d.x * d.y * d.z * (3.0 * z2 - 1.0);
}

float HardcodedSH5n1(const vec3 d) {
    const float z2 = d.z * d.z;
    const float z4 = z2 * z2;
    return -0.2416 * d.y * (39.3750 * z4 - 26.2500 * z2 + 1.8750);
}

float HardcodedSH50(const vec3 d) {
    const float z2 = d.z * d.z;
    const float z4 = z2 * z2;
    return 0.1170 * d.z * (63.0 * z4 - 70.0 * z2 + 15.0);
}

float HardcodedSH5p1(const vec3 d) {
    const float z2 = d.z * d.z;
    const float z4 = z2 * z2;
    return -0.2416 * d.x * (39.3750 * z4 - 26.2500 * z2 + 1.8750);
}

float HardcodedSH5p2(const vec3 d) {
    const float x2 = d.x * d.x;
    const float y2 = d.y * d.y;
    const float z2 = d.z * d.z;
    return 2.3968 * d.z * (x2 - y2) * (3.0 * z2 - 1.0);
}

float HardcodedSH5p3(const vec3 d) {
    const float x3 = d.x * d.x * d.x;
    const float y2 = d.y * d.y;
    const float z2 = d.z * d.z;
    return 0.0093 * (3.0 * d.x * y2 - x3)*(472.5000 * z2 - 52.5000);
}

float HardcodedSH5p4(const vec3 d) {
    const float x2 = d.x * d.x;
    const float y2 = d.y * d.y;
    const float x4 = x2 * x2;
    const float y4 = y2 * y2;
    return 2.0757 * d.z * (x4 - 6.0 * x2 * y2 + y4);
}

float HardcodedSH5p5(const vec3 d) {
    const float x2 = d.x * d.x;
    const float y2 = d.y * d.y;
    const float x4 = x2 * x2;
    const float y4 = y2 * y2;
    return -0.6564 * d.x * (x4 - 10.0 * x2 * y2 + 5.0 * y4);
}

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
    float s = 1;
    [[unroll, dependency_infinite]]
    for (int n = 1; n <= x; n++) {
        s *= n;
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
    float s = 1;
    float n = x;
    [[unroll, dependency_infinite]]
    while (n >= 2) {
        s *= n;
        n -= 2.0;
    }
    return s;
}











float evalLegendrePolynomial(const int l, const int m, const float x) {
    
    
    float pmm = 1.0;
    
    if (m > 0) {
        float sign = (m % 2 == 0 ? 1 : -1);
        pmm = sign * doubleFactorial(2 * m - 1) * pow(1 - x * x, m / 2.0);
    }

    if (l == m) {
        
        return pmm;
    }

    
    float pmm1 = x * (2 * m + 1) * pmm;
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

void toSphericalCoords(in vec3 dir, out float phi, out float theta) { 
    phi = atan(dir.y, dir.x); 
    theta = acos(dir.z);
}

float evalSH(const int l, const int m, const float phi, const float theta) {
    
    
    const float SQRT2 = 1.414213562373095f;

    const int absM = abs(m);
    const float kml = sqrt((2.0f * l + 1.0f) * factorial(l - absM) / (factorial(l + absM)));
    const float kmlTimesLPolyEval = kml * evalLegendrePolynomial(l, absM, cos(theta));
    if (m == 0) return kmlTimesLPolyEval;

    const float mTimesPhi = absM * phi;
    const float sh = SQRT2 * kmlTimesLPolyEval;
    if (m > 0) return sh * cos(mTimesPhi);
    else return sh * sin(mTimesPhi);
}

float evalSH(const int l, const int m, const vec3 dir) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    float phi, theta;
    toSphericalCoords(dir, phi, theta); 
    return evalSH(l, m, phi, theta);
}

uint getSHindex(const int l, const int m) {
    return (l * (l + 1) + m);
}

#ifdef SH_READ
float evalSHSum(const int order, const int dataOffset, const float phi, const float theta) { 
    float sum = 0.0f;
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            sum += evalSH(l, m, phi, theta) * SH_DATA_BUFFER_[dataOffset + getSHindex(l, m)];
        }
    }
    return sum;
}

vec3 evalSHSumRGB(const int order, const int dataOffset, const float phi, const float theta) { 
    vec3 color = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            color += evalSH(l, m, phi, theta) * vec3(
                SH_DATA_BUFFER_[dataOffset + idxX3 + 0],
                SH_DATA_BUFFER_[dataOffset + idxX3 + 1],
                SH_DATA_BUFFER_[dataOffset + idxX3 + 2]
            );
        }
    }
    return color;
}

vec3 evalSHSumRGB(const int order, const int dataOffset, const vec3 dir) { 
    float phi, theta;
    toSphericalCoords(dir, phi, theta); 
    return evalSHSumRGB(order, dataOffset, phi, theta);
}


vec3 evalInterSHSumRGB(const int order, const uvec3 dataOffsets, const vec3 bary, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            colour += evalSH(l, m, phi, theta) * vec3(
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]
            );
        }
    }
    return colour;
}
vec3 evalInterSHSumRGB(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir) { 
    float phi, theta;
    toSphericalCoords(dir, phi, theta); 
    return evalInterSHSumRGB(order, dataOffsets, bary, phi, theta);
}
#endif

#ifdef SH_WRITE




void rotateZHtoSH(const int order, const int dataOffset, const int zhOffset, const float phi, const float theta) {
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            SH_DATA_BUFFER_[dataOffset + getSHindex(l, m)] = sqrt(4.0 * M_PI / (2.0 * l + 1.0)) * SH_DATA_BUFFER_[zhOffset + l] * evalSH(l, m, phi, theta); 
        }
    }
}
#endif



void toHemiSphericalCoords(in vec3 dir, in vec3 normal, in vec3 tangent, out float phi, out float theta) { 
    const vec3 bitangent = cross(normal, tangent);
    phi = atan(dot(dir, bitangent), dot(dir, tangent));
    theta = acos(dot(dir, normal));
    if (theta < 0.0)      theta = 0.0;
    if (theta > 0.5 * M_PI) theta = 0.5 * M_PI;
}

float evalHSH(const int l, const int m, const float phi, const float theta) {
    
    
    const float SQRT2 = 1.414213562373095f;
    const int absM = abs(m);
    const float cos2theta = 2.0f * cos(theta) - 1.0f;

    const float kml = sqrt((2.0f * l + 1.0f) * factorial(l - absM) / (factorial(l + absM)));
    const float kmlTimesLPolyEval = kml * evalLegendrePolynomial(l, absM, cos2theta);
    if (m == 0) return kmlTimesLPolyEval;

    const float mTimesPhi = absM * phi;
    const float sh = SQRT2 * kmlTimesLPolyEval;
    if (m > 0) return sh * cos(mTimesPhi);
    else /* m < 0 */ return sh * sin(mTimesPhi);
}

float evalHSH(const int l, const int m, const vec3 dir, const vec3 normal, const vec3 tangent) { 
    float phi, theta;
    
    toHemiSphericalCoords(dir, normal, tangent, phi, theta); 
    return evalHSH(l, m, phi, theta);
}

#ifdef SH_READ
vec3 evalHSHSumRGB(const int order, const int dataOffset, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            colour += evalHSH(l, m, phi, theta) * vec3(
                SH_DATA_BUFFER_[dataOffset + idxX3],
                SH_DATA_BUFFER_[dataOffset + idxX3 + 1],
                SH_DATA_BUFFER_[dataOffset + idxX3 + 2]
            );
        }
    }
    return colour;
}

vec3 evalHSHSumRGB(const int order, const int dataOffset, const vec3 dir, const vec3 normal, const vec3 tangent) { 
    float phi, theta;
    
    toHemiSphericalCoords(dir, normal, tangent, phi, theta); 
    return evalHSHSumRGB(order, dataOffset, phi, theta);
}


vec3 evalInterHSHSumRGB(const int order, const uvec3 dataOffsets, const vec3 bary, const float phi, const float theta) { 
    vec3 colour = vec3(0.0f);
    [[unroll, dependency_infinite]]
    for (int l = 0; l <= order; l++) {
        [[unroll, dependency_infinite]]
        for (int m = -l; m <= l; m++) {
            const uint idxX3 = 3 * getSHindex(l, m);
            colour += evalHSH(l, m, phi, theta) * vec3(
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3    ] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3    ] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3    ],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 1] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 1] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 1],
                bary.x * SH_DATA_BUFFER_[dataOffsets.x + idxX3 + 2] + bary.y * SH_DATA_BUFFER_[dataOffsets.y + idxX3 + 2] + bary.z * SH_DATA_BUFFER_[dataOffsets.z + idxX3 + 2]
            );
        }
    }
    return colour;
}
vec3 evalInterHSHSumRGB(const int order, const uvec3 dataOffsets, const vec3 bary, const vec3 dir, const vec3 normal, const vec3 tangent) { 
    float phi, theta;
    
    toHemiSphericalCoords(dir, normal, tangent, phi, theta); 
    return evalInterHSHSumRGB(order, dataOffsets, bary, phi, theta);
}

#endif

#endif
