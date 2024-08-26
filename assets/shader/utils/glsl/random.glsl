
#ifndef GLSL_RANDOM_GLSL
#define GLSL_RANDOM_GLSL

#ifndef __cplusplus
	#extension GL_EXT_shader_explicit_arithmetic_types : require 
	

	#ifndef REF
		#define REF(type) inout type
	#endif
	#ifndef OREF
		#define OREF(type) out type
	#endif
#endif






float randomFloat(const vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898f, 78.233f))) * 43758.5453f);
}





float randomFloatTriDist(const vec2 uv) {
	
	
	float nrnd0 = randomFloat(uv);
	
	
	const float orig = nrnd0 * 2.0f - 1.0f;		
	nrnd0 = orig * inversesqrt(abs(orig));
	nrnd0 = max(-1.0f, nrnd0); 				
	
	
	return nrnd0 - sign(orig); 				
	
}

/*****************************/
/* Tiny Encryption Algorithm */
/*****************************/




uint32_t tea_init(uint32_t v0, uint32_t v1) {
	uint32_t sum = 0u;

	
	for (uint32_t n = 0u; n < 16u; n++) {
		sum += 0x9e3779b9U;
		v0 += ((v1 << 4u) + 0xa341316cU) ^ (v1 + sum) ^ ((v1 >> 5u) + 0xc8013ea4U);
		v1 += ((v0 << 4u) + 0xad90777dU) ^ (v0 + sum) ^ ((v0 >> 5u) + 0x7e95761eU);
	}

	return v0;
}



uint32_t tea_nextUInt(REF(uint32_t) seed)
{
	
	return (seed = 1664525u * seed + 1013904223u);
}


float tea_nextFloat(REF(uint32_t) seed)
{
	
	const uint32_t one = 0x3f800000;
	const uint32_t msk = 0x007fffff;
	return uintBitsToFloat(0x3f800000u | (0x007fffffu & tea_nextUInt(seed))) - 1.0f;
}
vec2 tea_nextFloat2(REF(uint32_t) seed) { return vec2(tea_nextFloat(seed), tea_nextFloat(seed)); }
vec3 tea_nextFloat3(REF(uint32_t) seed) { return vec3(tea_nextFloat(seed), tea_nextFloat(seed), tea_nextFloat(seed)); }
vec4 tea_nextFloat4(REF(uint32_t) seed) { return vec4(tea_nextFloat(seed), tea_nextFloat(seed), tea_nextFloat(seed), tea_nextFloat(seed)); }



float tea_nextFloatFast(REF(uint32_t) seed) {
    return uintBitsToFloat(tea_nextUInt(seed) & 0x00FFFFFFu) / uintBitsToFloat(0x01000000u);
}
vec2 tea_nextFloatFast2(REF(uint32_t) seed) { return vec2(tea_nextFloatFast(seed), tea_nextFloatFast(seed)); }
vec3 tea_nextFloatFast3(REF(uint32_t) seed) { return vec3(tea_nextFloatFast(seed), tea_nextFloatFast(seed), tea_nextFloatFast(seed)); }
vec4 tea_nextFloatFast4(REF(uint32_t) seed) { return vec4(tea_nextFloatFast(seed), tea_nextFloatFast(seed), tea_nextFloatFast(seed), tea_nextFloatFast(seed)); }

/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http:
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *       http:
 */

/*
 * This code is derived from the full C implementation, which is in turn
 * derived from the canonical C++ PCG implementation. The C++ version
 * has many additional features and is preferable if you can use C++ in
 * your project.
 */


uint32_t pcg32_nextUInt(REF(u64vec2) seed) {
    uint64_t oldstate = seed.x;
    seed.x = oldstate * 0x5851f42d4c957f2dUL + seed.y;
    uint32_t xorshifted = uint32_t(((oldstate >> 18ul) ^ oldstate) >> 27ul);
    uint32_t rot = uint32_t(oldstate >> 59ul);
    return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31u));
}


float pcg32_nextFloat(REF(u64vec2) seed) {
    
    
    const uint32_t u = (pcg32_nextUInt(seed) >> 9u) | 0x3f800000u;
    return uintBitsToFloat(u) - 1.0f;
}
vec2 pcg32_nextFloat2(REF(u64vec2) seed) { return vec2(pcg32_nextFloat(seed), pcg32_nextFloat(seed)); }
vec3 pcg32_nextFloat3(REF(u64vec2) seed) { return vec3(pcg32_nextFloat(seed), pcg32_nextFloat(seed), pcg32_nextFloat(seed)); }
vec4 pcg32_nextFloat4(REF(u64vec2) seed) { return vec4(pcg32_nextFloat(seed), pcg32_nextFloat(seed), pcg32_nextFloat(seed), pcg32_nextFloat(seed)); }


double pcg32_nextDouble(REF(u64vec2) seed) {
    
    
    const uint64_t u = (uint64_t(pcg32_nextUInt(seed)) << 20ul) | 0x3ff0000000000000ul;
    return uint64BitsToDouble(u) - 1.0;
}

/* vec2(state, inc) */
u64vec2 pcg32_init(uint64_t initstate, uint64_t initseq) {
	const uint64_t one = 1;
    initseq = max(one, initseq);
    u64vec2 s;
    s.x = 0u;
    s.y = (initseq << 1u) | 1u;
    pcg32_nextUInt(s);
    s.x += initstate;
    pcg32_nextUInt(s);
    return s;
}

#endif /* GLSL_RANDOM_GLSL */