#ifndef RANDOM_HLSL
#define RANDOM_HLSL

#ifdef __cplusplus
    #define UNROLL
#else /* HLSL */
    #define UNROLL [unroll]
#endif

namespace rng {
    
    
    float simple(const float2 co) {
        const float a = 12.9898f;
        const float b = 78.233f;
        const float c = 43758.5453f;
        const float dt = dot(co, float2(a, b));
        const float sn = fmod(dt, 3.14f);
        return frac(sin(sn) * c);
    }

    
    
    struct tea {
        uint32_t seed;
        
        
        
        void init(uint32_t v0, uint32_t v1) {
            uint32_t sum = 0u;
            UNROLL
            for (uint32_t n = 0u; n < 16u; n++) {
                sum += 0x9e3779b9;
                v0 += ((v1 << 4) + 0xa341316cU) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4U);
                v1 += ((v0 << 4) + 0xad90777dU) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761eU);
            }
            seed = v0;
        }

        
        
        uint32_t nextUInt() {
            return (seed = 1664525u * seed + 1013904223u);
        }
        
        
        float nextFloat() {
            return asfloat(0x3f800000u | (0x007fffffu & nextUInt())) - 1.0f;
        }
        float2 nextFloat2() { return float2(nextFloat(), nextFloat()); }
        float3 nextFloat3() { return float3(nextFloat(), nextFloat(), nextFloat()); }
        float4 nextFloat4() { return float4(nextFloat(), nextFloat(), nextFloat(), nextFloat()); }

        
        
        float nextFloatFast() {
            return asfloat(nextUInt() & 0x00ffffffu) / asfloat(0x01000000u);
        }
        float2 nextFloatFast2() { return float2(nextFloatFast(), nextFloatFast()); }
        float3 nextFloatFast3() { return float3(nextFloatFast(), nextFloatFast(), nextFloatFast()); }
        float4 nextFloatFast4() { return float4(nextFloatFast(), nextFloatFast(), nextFloatFast(), nextFloatFast()); }
    };

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
    struct pcg32 {
        uint64_t state;
        uint64_t inc;

        
        void init() {
            state = 0x853c49e6748fea9bULL;
            inc = 0xda3e39cb94b95bdbULL;
        }
        void init(uint64_t initstate, uint64_t initseq) {
            initseq = max(1ULL, initseq);
            state = 0U;
            inc = (initseq << 1ULL) | 1ULL;
            nextUInt();
            state += initstate;
            nextUInt();
        }

        
        uint32_t nextUInt() {
            const uint64_t oldstate = state;
            state = oldstate * 0x5851f42d4c957f2dULL + inc;
            const uint32_t xorshifted = uint32_t(((oldstate >> 18ULL) ^ oldstate) >> 27ULL);
            const uint32_t rot = uint32_t(oldstate >> 59ULL);
            return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31u));
        }

        
        float nextFloat() {
            
            
            return asfloat((nextUInt() >> 9u) | 0x3f800000u) - 1.0f;
        }

        float2 nextFloat2() { return float2(nextFloat(), nextFloat()); }
        float3 nextFloat3() { return float3(nextFloat(), nextFloat(), nextFloat()); }
        float4 nextFloat4() { return float4(nextFloat(), nextFloat(), nextFloat(), nextFloat()); }

        
        double nextDouble() {
            
            
            const uint64_t u = (uint64_t(nextUInt()) << 20ULL) | 0x3ff0000000000000ULL;
            return asdouble(uint32_t(u), uint32_t(u >> 32U)) - 1.0;
        }

        double2 nextDouble2() { return double2(nextDouble(), nextDouble()); }
        double3 nextDouble3() { return double3(nextDouble(), nextDouble(), nextDouble()); }
        double4 nextDouble4() { return double4(nextDouble(), nextDouble(), nextDouble(), nextDouble()); }
    };
}
#endif
