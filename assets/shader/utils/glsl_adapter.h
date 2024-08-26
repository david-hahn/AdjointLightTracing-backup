#ifndef GLSL_ADAPTER_H
#define GLSL_ADAPTER_H

#define GLM_FORCE_SWIZZLE
#define GLM_SWIZZLE_XYZW
#define GLM_SWIZZLE_RGBA
#define GLM_SWIZZLE_STQP
#define GLM_FORCE_INTRINSICS
#include <glm/glm.hpp>
using namespace glm;


#define REF(type) type &
#define OREF(type) type &
#define IREF(type) type

double uint64BitsToDouble(uint64_t bits) { return *(double *)&bits; }

#endif /* GLSL_ADAPTER_H */