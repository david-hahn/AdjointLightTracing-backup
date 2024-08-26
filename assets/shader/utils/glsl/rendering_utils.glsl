
#ifndef GLSL_RENDERING_UTILS
#define GLSL_RENDERING_UTILS

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

#ifndef DOT01
#define DOT01(a,b) max(0.0f, dot(a, b))
#endif

#ifndef BITS_SET
#define BITS_SET(a,b) ((a&b)>0)
#endif





vec3 halfvector(const vec3 wi, const vec3 wo) {
#ifdef DEBUG_PRINT_BXDF_CHECK_NAN
	if (dot(wi, wo) == 0.0f) debugPrintfEXT("halfvector dot(wi,wo) == 0");
#endif
	return normalize(wi + wo);
}




vec3 halfvector(const vec3 wi, const vec3 wo, const float eta_i, const float eta_o) {
	return normalize(-eta_i * wi - eta_o * wo);
}





#define WATT_TO_RADIANT_INTENSITY(watt) (watt * M_INV_4PI)



#define ATTENUATION(distance) (1.0 / (distance * distance))
#define ATTENUATION_RANGE(distance,range) (range == 0.0f ? ((1.0f) / pow(distance,2)) : clamp(1.0f - pow( distance / range, 4.0f ), 0.0f, 1.0f ) / pow(distance, 2.0f))

#define ATTENUATION_RANGE_UE4(distance,range) (range == 0.0f ? ((1.0f) / (pow(distance, 2.0f)+1)) : pow(clamp(1.0f - pow( distance / range, 4.0f ), 0.0f, 1.0f ),2) / (pow(distance, 2.0f)+1))



float srgbToLinear(const float srgb)
{
	if (srgb <= 0.04045f) return srgb / 12.92f;
	else return pow((srgb + 0.055f) / 1.055f, 2.4f);
}
vec3 srgbToLinear(const vec3 srgb)
{
	return vec3(srgbToLinear(srgb.x), srgbToLinear(srgb.y), srgbToLinear(srgb.z));
}
vec4 srgbToLinear(const vec4 srgb)
{
	return vec4(srgbToLinear(srgb.xyz), srgb.w);
}
float linearToSRGB(const float linear)
{
	if (linear <= 0.0031308f) return 12.92f * linear;
	else return 1.055f * pow(linear, 1.0f / 2.4f) - 0.055f;
}
vec3 linearToSRGB(const vec3 linear)
{
	return vec3(linearToSRGB(linear.x), linearToSRGB(linear.y), linearToSRGB(linear.z));
}
vec4 linearToSRGB(const vec4 linear)
{
	return vec4(linearToSRGB(linear.xyz), linear.w);
}



vec3 srgbToLinearFast(const vec3 srgb)
{
	return pow(srgb, vec3(2.2f));
}
vec4 srgbToLinearFast(const vec4 srgb)
{
	return vec4(srgbToLinearFast(srgb.xyz), srgb.a);
}
vec3 linearToSRGBFast(const vec3 linear)
{
	return pow(linear, vec3(1.0 / 2.2f));
}
vec4 linearToSRGBFast(const vec4 linear)
{
	return vec4(linearToSRGBFast(linear.xyz), linear.a);
}




float luminance(const vec3 color)
{
	return dot(vec3(0.2126, 0.7152, 0.0722), color);
	
}


vec3
clampColor(const vec3 color, const float max_val)
{
	float m = max(color.r, max(color.g, color.b));
	if (m < max_val) return color;
	return color * (max_val / m);
}
float saturate(const float value) {
	return clamp(value, 0.0f, 1.0f);
}

float _log10(const float x) {
   const float oneDivLog10 = 0.434294481903252f;
   return log(x) * oneDivLog10;
}


vec2 worldSpaceToScreenSpace(const vec4 ws, const mat4 mvp, const vec2 screen_size) {
	vec4 clip_space = mvp * ws;
	vec4 ndc = clip_space / clip_space.w;
	vec2 screen_space = ((ndc.xy + vec2(1.0)) / vec2(2.0)) * screen_size;
	return screen_space;
}

mat4 rotationMatrix(vec3 axis, const float angle)
{
	axis = normalize(axis);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;

	return mat4(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s, 0.0,
		oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0,
		oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c, 0.0,
		0.0, 0.0, 0.0, 1.0);
}



void createONB(const vec3 normal, OREF(vec3) tangent, OREF(vec3) bitangent)
{
	const float s = sign(normal.z);
	const float a = 1.0f / (1.0f + abs(normal.z));
	const float b = -s * normal.x * normal.y * a;
	tangent = vec3(1.0f - normal.x * normal.x * a, b, -s * normal.x);
	bitangent = vec3(b, s * 1.0f - normal.y * normal.y * a, -normal.y);

	
	
	
	
	
	
	
	
	
	
	
	
	
}
mat3 createONBMat(const vec3 normal)
{
	vec3 tangent, bitangent;
	createONB(normal, tangent, bitangent);
	
	
	
	return mat3(tangent, bitangent, normal);
}
vec3 toLocalSpace(const mat3 tbn /*mat3(t,b,n)*/, const vec3 v) {
	return v * tbn;
}
vec3 toGlobalSpace(const mat3 tbn /*mat3(t,b,n)*/, const vec3 v) {
	return tbn * v;
}
vec3 toWorld(const vec3 T, const vec3 B, const vec3 N, const vec3 V)
{
	return V.x * T + V.y * B + V.z * N;
}

vec3 toLocal(const vec3 T, const vec3 B, const vec3 N, const vec3 V)
{
	return vec3(dot(V, T), dot(V, B), dot(V, N));
}




vec3 getPerpendicularVector(const vec3 u)
{
	const vec3 a = abs(u);
	const uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
	const uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
	const uint zm = 1 ^ (xm | ym);
	return normalize( cross(u, vec3(xm, ym, zm)) );
}


void getTBVector(const vec3 v0, const vec3 v1, const vec3 v2,
				const vec2 uv0, const vec2 uv1, const vec2 uv2,
				OREF(vec3) tangent, OREF(vec3) bitangent)
{
	const vec3 dv1 = v1-v0;
	const vec3 dv2 = v2-v0;

	const vec2 duv1 = uv1-uv0;
	const vec2 duv2 = uv2-uv0;

	float r = 1.0f / (duv1.x * duv2.y - duv1.y * duv2.x);
	tangent = (dv1 * duv2.y - dv2 * duv1.y) * r;
	bitangent = (dv2 * duv1.x - dv1 * duv2.x) * r;
}
void getTBVector(const vec3 v0, const vec3 v1, const vec3 v2,
				const vec2 uv0, const vec2 uv1, const vec2 uv2,
				OREF(vec3) tangent)
{
	vec3 bitangent;
	getTBVector(v0, v1, v2, uv0, uv1, uv2, tangent, bitangent);
}



vec3 tangentSpaceToWorldSpace(const vec3 v, const vec3 t, const vec3 b, const vec3 n) {
	
	
	
	
	return mat3(t, b, n) * v;
}
vec3 worldSpaceToTangentSpace(const vec3 v, const vec3 t, const vec3 b, const vec3 n) {
	
	
	
	
	return transpose(mat3(t, b, n)) * v;
}

vec3 tangentSpaceToWorldSpace(const vec3 v, const vec3 n) {
	
	const vec3 t = normalize(getPerpendicularVector(n));
	
	const vec3 b = normalize(cross(n, t));
	
	
	
	
	
	return mat3(t, b, n) * v;
}
vec3 worldSpaceToTangentSpace(const vec3 v, const vec3 n) {
	
	const vec3 t = normalize(getPerpendicularVector(n));
	
	const vec3 b = normalize(cross(n, t));
	
	
	
	
	return transpose(mat3(t, b, n)) * v;
}

vec3 nTangentSpaceToWorldSpace(const vec3 n_ts, const float scale, const vec3 t, const vec3 b, const vec3 n) {
	return normalize(tangentSpaceToWorldSpace(normalize((n_ts * 2.0f - 1.0f) * vec3(scale, scale, 1)), t, b, n));
}
vec3 nTangentSpaceToWorldSpace(const vec3 n_ts, const float scale, const vec3 n) {
	
	const vec3 t = normalize(getPerpendicularVector(n));
	
	const vec3 b = normalize(cross(n, t));
	return normalize(tangentSpaceToWorldSpace(normalize((n_ts * 2.0f - 1.0f) * vec3(scale, scale, 1)), t, b, n));
}

/*
** Convertions
*/


vec3 sphericalToCartesian(const float r, const float theta, const float phi) {
	return vec3(r * sin(theta) * cos(phi), r * sin(theta) * sin(phi), r * cos(theta));
	
	
}

vec3 sphericalToCartesian(const vec3 v) {
	return sphericalToCartesian(v.x, v.y, v.z);
}
vec3 cartesianToSpherical(const vec3 v) {
	const vec3 v2 = v * v;
	const float r = sqrt(v2.x + v2.y + v2.z);
	const float theta = atan(sqrt(v2.x + v2.y), v.z);
	const float phi = atan(v.y, v.x);
	
	
	return vec3(r, theta, phi);
}






float solidAngleToArea(const float area, const float dist, const float cosTheta) {
	return (area * abs(cosTheta)) / (dist * dist);
}
float areaToSolidAngle(const float area, const float dist, float cosTheta) {
	return (dist * dist) / (area * abs(cosTheta));
}

/*
** Post Processing
*/


vec3 dither(const vec3 col, const float strength, const float rnd)
{
	return col + (rnd * 0.0033f * strength);
}

uvec2 linTo2D(uint _index, uint _row_size) {
	uint j = _index / _row_size;
	uint i = _index % _row_size;
	return uvec2(i, j);
}
uint linFrom2D(uint _i, uint _j, uint _row_size) {
	return (_j * _row_size) + _i;
}

vec3 hash3( uint n ) 
{
    
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    uvec3 k = n * uvec3(n,n*16807U,n*48271U);
    return vec3( k & uvec3(0x7fffffffU))/float(0x7fffffff);
}
#endif /* GLSL_RENDERING_UTILS */