#ifndef BSDFS_HLSL
#define BSDFS_HLSL

#ifndef M_PI
#define M_PI 3.14159265358979
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717959
#endif
#ifndef M_4PI
#define M_4PI 12.566370614359172
#endif
#ifndef M_INV_PI
#define M_INV_PI 0.318309886183791
#endif
#ifndef M_INV_2PI
#define M_INV_2PI 0.159154943091895
#endif
#ifndef M_INV_4PI
#define M_INV_4PI 0.0795774715459477
#endif
#ifndef M_PI_DIV_2
#define M_PI_DIV_2 1.5707963267949
#endif
#ifndef M_PI_DIV_4
#define M_PI_DIV_4 0.785398163397448
#endif

namespace microfacet {
    namespace gtr {
        
        
        
        
        
        float d(const float MdotN, const float alpha, const float gamma) {
            if (MdotN <= 0.0) return 0.0;
            const float alpha2 = alpha * alpha;
            const float t = 1.0 + (alpha2 - 1.0) * MdotN * MdotN;
            return alpha2 / (M_PI * pow(t, gamma));
        }
        float pdf(const float MdotN, const float alpha, const float gamma) {
            return d(MdotN, alpha, gamma) * MdotN;
        }
        
        
        
        float g1(const float Mdot_, const float Ndot_, const float alpha) {
            if ((Mdot_ / Ndot_) <= 0.0f) return 0.0f;
            const float alpha2 = alpha * alpha;
            const float cos_theta2 = Ndot_ * Ndot_;
            const float tan_theta2 = (1.0 - cos_theta2) / cos_theta2;
            return 2.0 / (1.0 + sqrt(1.0 + alpha2 * tan_theta2));
        }
        float g(const float MdotV, const float MdotL, const float NdotV, const float NdotL, const float alpha) {
            const float g1v = g1(MdotV, NdotV, alpha);
            const float g1l = g1(MdotL, NdotL, alpha);
            return g1v * g1l;
        }
    }
    namespace gtr2 {
        float d(const float MdotN, const float alpha) {
            return gtr::d(MdotN, alpha, 2.0);
        }
        float pdf(const float MdotN, const float alpha) {
            return gtr::pdf(MdotN, alpha, 2.0) * MdotN;
        }
    }
    namespace ggx {
        float d(const float MdotN, const float alpha) {
            return gtr::d(MdotN, alpha, 2.0);
        }
        float pdf(const float MdotN, const float alpha) {
            return gtr::pdf(MdotN, alpha, 2.0) * MdotN;
        }
    }

    
    namespace model {
        
        
        
        
        
        
        float cook_torrance(const float NdotL, const float NdotV) {
            
            return 1.0 / (M_PI * abs(NdotL) * abs(NdotV));
        }
        
        
        
        
        float torrance_sparrow(const float wiDotHr, const float woDotHr) {
            if (wiDotHr * woDotHr == 0) return 0;
            return 1.0 / (4.0 * abs(wiDotHr) * abs(woDotHr));
        }
        float torrance_sparrow(const float wiDotHt, const float woDotHt, const float wiDotN, 
                const float woDotN, const float eta_i, const float eta_o) {
            const float a = (abs(wiDotHt) * abs(woDotHt)) / (abs(wiDotN) * abs(woDotN));
            const float b = (eta_i * wiDotHt + eta_o * woDotHt);
            if (b == 0.0) return 0.0;
            return a * ((eta_o * eta_o) / (b * b));
        }
        
        float ggx(const float wiDotHr, const float woDotHr) {
            return torrance_sparrow(wiDotHr, woDotHr);
        }
        float ggx(const float wiDotHt, const float woDotHt, const float wiDotN,
                  const float woDotN, const float eta_i, const float eta_o) {
            return torrance_sparrow(wiDotHt, woDotHt, wiDotN, woDotN, eta_i, eta_o);
        }
    }
}

namespace bsdfs {
    namespace lambert {
        float3 eval(const float3 wo, const float3 n, const float3 albedo) {
            float3 result = albedo * M_INV_PI;
        #ifndef BSDFS_EVAL_NO_COS
            result *= max(dot(n, wo), 0.0f);
        #endif
            return result;
        }

        
        
        
        float3 disney_eval(const float wiDotN, const float woDotN, const float VdotH, const float3 albedo, const float roughness) {
            const float fd90 = 0.5 + 2.0 * roughness * VdotH * VdotH;
            const float fnl = 1.0 + (fd90 - 1.0) * pow(1.0 - wiDotN, 5.0);
            const float fnv = 1.0 + (fd90 - 1.0) * pow(1.0 - woDotN, 5.0);
            return albedo * M_INV_PI * fnl * fnv;
        }

        float pdf(const float3 wo, const float3 n) {
            return max(0.0f, dot(wo, n)) * M_INV_PI;
        }

        
        
        
        
        
        
        
        
    }
}
#endif
