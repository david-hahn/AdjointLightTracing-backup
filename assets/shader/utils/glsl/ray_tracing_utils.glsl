
#ifndef GLSL_RAY_TRACING_UTILS
#define GLSL_RAY_TRACING_UTILS

#ifndef __cplusplus
	#ifndef REF
		#define REF(type) inout type
	#endif
	#ifndef OREF
		#define OREF(type) out type
	#endif
#endif

#ifndef M_PI
#define M_PI 3.14159265358979f
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717959f
#endif
#ifndef M_4PI
#define M_4PI 12.566370614359172f
#endif
#ifndef M_INV_PI
#define M_INV_PI 0.318309886183791f
#endif
#ifndef M_INV_2PI
#define M_INV_2PI 0.159154943091895f
#endif
#ifndef M_INV_4PI
#define M_INV_4PI 0.0795774715459477f
#endif
#ifndef M_PI_DIV_2
#define M_PI_DIV_2 1.5707963267949f
#endif
#ifndef M_PI_DIV_4
#define M_PI_DIV_4 0.785398163397448f
#endif

#ifndef SAVE_SQRT
#define SAVE_SQRT(x) x >= 0 ? sqrt(x) : x
#endif

vec3 computeBarycentric(const vec2 hitAttribute) {
	return vec3(1.0f - hitAttribute.x - hitAttribute.y, hitAttribute.x, hitAttribute.y);
}

vec3 computeBarycentric2(const mat3 v, const vec3 ray_origin, const vec3 ray_direction)
{
	const vec3 edge1 = v[1] - v[0];
	const vec3 edge2 = v[2] - v[0];

	const vec3 pvec = cross(ray_direction, edge2);

	const float det = dot(edge1, pvec);
	const float inv_det = 1.0f / det;

	const vec3 tvec = ray_origin - v[0];
	const vec3 qvec = cross(tvec, edge1);

	const float alpha = dot(tvec, pvec) * inv_det;
	const float beta = dot(ray_direction, qvec) * inv_det;

	return vec3(1.0f - alpha - beta, alpha, beta);
}

/*
** SAMPLING
*/
int sampleRangeUniform(const int rmin, const int rmax, const float rand) {
	return int(((rmax+1) - rmin) * rand + rmin);
}

vec3 sampleUnitTriangleUniform(vec2 uv) {


	
	
	


	if (uv.y > uv.x) {
		uv.x *= 0.5f;
		uv.y -= uv.x;
	} else {
		uv.y *= 0.5f;
		uv.x -= uv.y;
	}
	return vec3(uv.x, uv.y, 1.0f - uv.x - uv.y);
}



vec3 orthogonalize(const vec3 a, const vec3 b) {
	
	return normalize(b - dot(a, b) * a);
}
vec3 slerp(const vec3 start, const vec3 end, const float percent)
{
	
	float cosTheta = dot(start, end);
	
	
	
	cosTheta = clamp(cosTheta, -1.0f, 1.0f);
	
	
	
	const float theta = acos(cosTheta) * percent;
	const vec3 RelativeVec = normalize(end - start * cosTheta);
	
								
	return ((start * cos(theta)) + (RelativeVec * sin(theta)));
}





void sampleSphericalTriangle(const vec3 A, const vec3 B, const vec3 C, const float Xi1, const float Xi2, OREF(vec3) w, OREF(float) wPdf) {
	
	vec3 BA = orthogonalize(A, B - A);
	vec3 CA = orthogonalize(A, C - A);
	vec3 AB = orthogonalize(B, A - B);
	vec3 CB = orthogonalize(B, C - B);
	vec3 BC = orthogonalize(C, B - C);
	vec3 AC = orthogonalize(C, A - C);
	float alpha = acos(clamp(dot(BA, CA), -1.0f, 1.0f));
	float beta = acos(clamp(dot(AB, CB), -1.0f, 1.0f));
	float gamma = acos(clamp(dot(BC, AC), -1.0f, 1.0f));

	
	float a = acos(clamp(dot(B, C), -1.0f, 1.0f));
	float b = acos(clamp(dot(C, A), -1.0f, 1.0f));
	float c = acos(clamp(dot(A, B), -1.0f, 1.0f));

	float area = alpha + beta + gamma - M_PI;

	
	float area_S = Xi1 * area;

	
	float p = sin(area_S - alpha);
	float q = cos(area_S - alpha);

	
	float u = q - cos(alpha);
	float v = p + sin(alpha) * cos(c);

	
	float s = (1.0 / b) * acos(clamp(((v * q - u * p) * cos(alpha) - v) / ((v * p + u * q) * sin(alpha)), -1.0f, 1.0f));

	
	vec3 C_s = slerp(A, C, s);

	
	float t = acos(1.0 - Xi2 * (1.0 - dot(C_s, B))) / acos(dot(C_s, B));

	
	vec3 P = slerp(B, C_s, t);

	w = P;
	wPdf = 1.0 / area;
}






































vec2 sampleUnitSquareUniform(const vec2 uv) {
	return uv;
}


vec2 sampleUnitDiskUniform(const vec2 uv) {
	const float r = sqrt(uv.x);
	const float theta = M_2PI * uv.y; 		
	return r * vec2(cos(theta), sin(theta));
}


vec2 sampleUnitDiskConcentric(const vec2 uv) {
	const vec2 offset = 2.0f * uv - 1.0f;
	if (all(equal(offset, vec2(0.0f)))) return vec2(0.0f);

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
	return r * vec2(cos(theta), sin(theta));
}

vec3 sampleUnitSphereUniform(const vec2 uv) {
	const float z = 1.0f - 2.0f * uv.x; 			
	const float r = sqrt(max(0.0f, 1.0f - z * z));
	const float theta = M_2PI * uv.y;  		
	return vec3(r * cos(theta), r * sin(theta), z);
}
vec3 sampleUnitHemisphereUniform(const vec2 uv) {
	const float z = uv.x; 							
	const float r = sqrt(max(0.0f, 1.0f - z * z));
	const float theta = M_2PI * uv.y; 		
	return vec3(r * cos(theta), r * sin(theta), z);
}



vec3 sampleUnitHemisphereCosine(const vec2 uv) {
	const vec2 uDisk = sampleUnitDiskUniform(uv);
	return vec3(uDisk.x, uDisk.y, sqrt(max(0.0f, 1.0f - uDisk.x * uDisk.x - uDisk.y * uDisk.y)));
}


vec3 sampleUnitConeUniform(const vec2 uv, const float cosThetaMax) {
	const float cosTheta = (1.0f - uv.x) + uv.x * cosThetaMax;
	const float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
	const float phi = uv.y * M_2PI;
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}


vec3 sampleVonMisesFisher(const vec2 uv, const float kappa) {
	const float sy = max(1.0f - uv.y, 1e-6f);
    const float cos_theta = 1.0f + log(fma(1.0f - sy, exp(-2.0f * kappa), sy)) / kappa;
	const float temp = M_2PI * uv.x;
	const float s = sin(temp);
	const float c = cos(temp);
	
    const float sin_theta = SAVE_SQRT(1.0f - sqrt(cos_theta));
    const vec3 result = vec3(c * sin_theta, s * sin_theta, cos_theta);

	
	if(kappa == 0.0f) return sampleUnitSphereUniform(uv);
	return result;
}

#define PDF_RANGE_UNIFORM(LENGTH) (1.0f/(LENGTH))
#define PDF_TRIANGLE_UNIFORM(area) (1.0f/(area))
#define PDF_SQUARE_UNIFORM(area) (1.0f/(area))
#define PDF_DISK_UNIFORM(area) (1.0f/(area))
#define PDF_ELLIPSE_UNIFORM(radius) (1.0f/(M_PI * radius.x * radius.y))
#define PDF_SPHERE_UNIFORM(radius) (1.0f/(M_4PI * radius * radius))
#define PDF_HEMISPHERE_UNIFORM(radius) (1.0f/(M_2PI * radius * radius))
#define PDF_UNIT_HEMISPHERE_UNIFORM (M_INV_2PI)
#define PDF_UNIT_HEMISPHERE_COSINE(cosTheta) (cosTheta * M_INV_PI)
#define PDF_UNIT_CONE_UNIFORM(cosThetaMax) (1.0f / (M_2PI * (1.0f - cosThetaMax)))
float PDF_VON_MISES_FISHER(const float cosTheta, float kappa) {
	if(kappa > 0.0f) return exp(kappa * (cosTheta - 1.0f)) * (kappa * M_INV_2PI) / (1.0f - exp(-2.0f * kappa));
	else M_INV_4PI;
}



vec3 offsetRayToAvoidSelfIntersection(const vec3 p, const vec3 n)
{
	const float _origin = 1.0f / 32.0f;
	const float _float_scale = 1.0f / 65536.0f;
	const float _int_scale = 256.0f;

	const ivec3 of_i = ivec3(_int_scale * n.x, _int_scale * n.y, _int_scale * n.z);

	const vec3 p_i = vec3(
		intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
		intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
		intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return vec3(
		abs(p.x) < _origin ? p.x + _float_scale * n.x : p_i.x,
		abs(p.y) < _origin ? p.y + _float_scale * n.y : p_i.y,
		abs(p.z) < _origin ? p.z + _float_scale * n.z : p_i.z);
}




float misHeuristic(const float pdfA, const float pdfB, const float beta) {
	const float pdfABeta = pow(pdfA, beta);
	const float pdfBBeta = pow(pdfB, beta);
	const float pdfAB = pdfABeta + pdfBBeta;
	return pdfABeta / pdfAB;
}
float misHeuristic(const float pdfA, const float pdfB, const float pdfC, const float beta) {
	const float pdfABeta = pow(pdfA, beta);
	const float pdfBBeta = pow(pdfB, beta);
	const float pdfCBeta = pow(pdfC, beta);
	const float pdfABC = pdfABeta + pdfBBeta + pdfCBeta;
	return pdfABeta / pdfABC;
}
float misBalanceHeuristic(const float pdfA, const float pdfB) {
	return misHeuristic(pdfA, pdfB, 1);
}
float misBalanceHeuristic(const float pdfA, const float pdfB, const float pdfC) {
	return misHeuristic(pdfA, pdfB, pdfC, 1);
	
	
}
float misPowerHeuristic(const float pdfA, const float pdfB) {
	return misHeuristic(pdfA, pdfB, 2);
	
	
	
	
}

bool intersectSphere(const vec3 origin, const vec3 direction, const vec3 center, const float radius, const float tMin, const float tMax, OREF(float) t)
{
	
	const vec3 oc = origin - center;
	const float a = dot(direction, direction);
	const float b = dot(oc, direction);
	const float c = dot(oc, oc) - radius * radius;
	const float discriminant = b * b - a * c;

	if (discriminant >= 0)
	{
		const float t1 = (-b - sqrt(discriminant)) / a;
		const float t2 = (-b + sqrt(discriminant)) / a;

		if ((tMin <= t1 && t1 < tMax) || (tMin <= t2 && t2 < tMax))
		{
			t = (tMin <= t1 && t1 < tMax) ? t1 : t2;
			return true;
		}
	}
	return false;
}

#endif /* GLSL_RAY_TRACING_UTILS */
