#ifndef HLSL_ADAPTER_H
#define HLSL_ADAPTER_H

#define GLM_FORCE_SWIZZLE
#define GLM_SWIZZLE_XYZW
#define GLM_SWIZZLE_RGBA
#define GLM_SWIZZLE_STQP
#define GLM_FORCE_INTRINSICS
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/extended_min_max.hpp>
using namespace glm;
#define frac fract

#include <cstdint>

float asfloat(uint32_t bits) { return *(float *)&bits; }
double asdouble(uint32_t low_bits, uint32_t high_bits) { 
    const uint64_t bits = (((uint64_t)high_bits) << 32ULL) | ((uint64_t)low_bits);
    return *(double *)&bits; }

#endif