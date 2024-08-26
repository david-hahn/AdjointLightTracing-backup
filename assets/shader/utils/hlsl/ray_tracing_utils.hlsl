#ifndef RAY_TRACING_UTILS_HLSL
#define RAY_TRACING_UTILS_HLSL

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

float3 computeBarycentric(const float2 hitAttribute) {
	return float3(1.0 - hitAttribute.x - hitAttribute.y, hitAttribute.x, hitAttribute.y);
}

float3 computeBarycentric2(float3x3 v, float3 ray_origin, float3 ray_direction)
{
	const float3 edge1 = v[1] - v[0];
	const float3 edge2 = v[2] - v[0];

	const float3 pvec = cross(ray_direction, edge2);

	const float det = dot(edge1, pvec);
	const float inv_det = 1.0 / det;

	const float3 tvec = ray_origin - v[0];
	const float3 qvec = cross(tvec, edge1);

	const float alpha = dot(tvec, pvec) * inv_det;
	const float beta = dot(ray_direction, qvec) * inv_det;

	return float3(1.0 - alpha - beta, alpha, beta);
}

namespace sampling {
	int range_uniform(const int min, const int max, const float rng) {
		return int((max - min) * rng + min);
	}
	
	
	float3 unit_triangle_uniform(float2 uv) {
		const float sqrt_u = sqrt(uv.x);
		return float3(1.0 - sqrt_u, sqrt_u * (1.0 - uv.y), sqrt_u * uv.y);
	}
	float2 unit_square_uniform(const float2 uv) {
		return uv;
	}
	
	
	float2 unit_disk_uniform(const float2 uv) {
		const float r = sqrt(uv.x);
		const float theta = M_2PI * uv.y; 		
		return r * float2(cos(theta), sin(theta));
	}
	
	
	float2 unit_disk_concentric(const float2 uv) {
		const float2 offset = 2.0 * uv - 1.0;
		if (all((offset == float2(0.0, 0.0)))) return float2(0.0, 0.0);

		float r;
		float theta;
		if (abs(offset.x) > abs(offset.y)) {
			r = offset.x;
			theta = M_PI_DIV_4 * (offset.y / offset.x);
		}
		else {
			r = offset.y;
			theta = M_PI_DIV_2 - M_PI_DIV_4 * (offset.x / offset.y);
		}
		return r * float2(cos(theta), sin(theta));
	}
	
	float3 unit_sphere_uniform(const float2 uv) {
		const float z = 1.0 - 2.0 * uv.x; 			
		const float r = sqrt(max(0.0, 1.0 - z * z));
		const float theta = M_2PI * uv.y;  		
		return float3(r * cos(theta), r * sin(theta), z);
	}
	float3 unit_hemisphere_uniform(const float2 uv) {
		const float z = uv.x; 							
		const float r = sqrt(max(0.0, 1.0 - z * z));
		const float theta = M_2PI * uv.y; 		
		return float3(r * cos(theta), r * sin(theta), z);
	}
	
	
	
	float3 unit_hemisphere_cosine(const float2 uv) {
		const float2 uDisk = unit_disk_uniform(uv);
		return float3(uDisk.x, uDisk.y, sqrt(max(0.0, 1.0 - uDisk.x * uDisk.x - uDisk.y * uDisk.y)));
	}
	
	float3 unit_cone_uniform(const float2 uv, const float cosThetaMax) {
		const float cosTheta = (1.0 - uv.x) + uv.x * cosThetaMax;
		const float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
		const float phi = uv.y * M_2PI;
		return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	}

	namespace pdf {
		float range_uniform(const float length) 			{ return 1.0 / length; }
		float triangle_uniform(const float area) 			{ return 1.0 / area; }
		float square_uniform(const float area) 				{ return 1.0 / area; }
		float disk_uniform(const float radius) 				{ return 1.0 / (M_PI * radius * radius); }
		float ellipse_uniform(const float2 radius) 			{ return 1.0 / (M_PI * radius.x * radius.y); }
		float sphere_uniform(const float radius) 			{ return 1.0 / (M_4PI * radius * radius); }
		float hemisphere_uniform(const float radius) 		{ return 1.0 / (M_2PI * radius * radius); }
		float unit_hemisphere_uniform() 					{ return M_INV_2PI; }
		float unit_hemisphere_cosine(const float cosTheta) 	{ return cosTheta * M_INV_PI; }
		float unit_cone_uniform(const float cosThetaMax) 	{ return 1.0 / (M_2PI * (1.0 - cosThetaMax)); }
	}
}

namespace mis {
	
	
	
	float balance_heuristic(const float pdf, const float pdfOther) {
		return pdf / (pdf + pdfOther);
	}
	float power_heuristic(const float pdf, const float pdfOther, const float beta = 2) {
		const float pdfBeta = pow(pdf, beta);
		const float pdfOtherBeta = pow(pdfOther, beta);
		return pdfBeta / (pdfBeta + pdfOtherBeta);
	}
	
	float cutoff_heuristic(const float pdf, const float pdfOther, const float alpha) {
		const float pdfMax = max(pdf, pdfOther) * alpha;
		if(pdf < pdfMax) return 0.0;
		const float denom = (pdf >= pdfMax ? pdf : 0.0) + (pdfOther >= pdfMax ? pdfOther : 0.0);
		return pdf / denom;
	}
}

#endif
