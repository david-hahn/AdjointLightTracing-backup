#ifndef GLSL_TONE_MAPPING
#define GLSL_TONE_MAPPING

#define TM_NONE 				0
#define TM_REINHARD 			1
#define TM_REINHARD_EXTENDED 	2
#define TM_UNCHARTED2 			3
#define TM_UCHIMURA 			4
#define TM_LOTTES 				5
#define TM_FILMIC 				6
#define TM_ACES 				7

#ifdef GLSL
#define APPLY_EXPOSURE(c,e) (c*e)


vec3 rtt_and_odt_fit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 T_ACES(const vec3 color) {
	mat3 aces_input_matrix = mat3(	0.59719f, 0.35458f, 0.04823f,
									0.07600f, 0.90834f, 0.01566f,
									0.02840f, 0.13383f, 0.83777f);
	mat3 aces_output_matrix = mat3( 1.60475f, -0.53108f, -0.07367f,
									-0.10208f,  1.10813f, -0.00605f,
									-0.00327f, -0.07276f,  1.07602f);
    return clamp(rtt_and_odt_fit(color * aces_input_matrix) * aces_output_matrix, 0.0f, 1.0f);
}


vec3 T_ACES_Approximation(vec3 color)
{
	color *= 0.6f;
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0f, 1.0f);
}



vec3 T_Filmic(vec3 color) {
	color = max(vec3(0.0), color - 0.004);
	return (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
}

vec3 T_Filmic_Hejl2015(const vec3 color, const float white) {
	vec4 vh = vec4(color, white);	
	vec4 va = (1.425f * vh) + 0.05f;		
	vec4 vf = ((vh * va + 0.004f) / ((vh * (va + 0.55f) + 0.0491f))) - 0.0821f;
	return vf.rgb / vf.www;	
}

vec3 T_Reinhard(const vec3 color) {
	float lum_c = dot(vec3(0.2126, 0.7152, 0.0722), color);
	float lum_r = lum_c / (1.0 + lum_c);
	return color * (lum_r / lum_c);
}
vec3 T_Reinhard(const vec3 color, const float white) {
	float lum_c = dot(vec3(0.2126, 0.7152, 0.0722), color);
	float lum_r = (lum_c * (1.0 + (lum_c / (white * white)))) / (1.0 + lum_c);
	return color * (lum_r / lum_c);
}

vec3 Uncharted2_Tonemap(const vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}
vec3 T_Uncharted2(const vec3 color)
{
	const float W = 11.2;
	float exposureBias = 2.0;
	vec3 curr = Uncharted2_Tonemap(exposureBias * color);
	vec3 whiteScale = 1.0 / Uncharted2_Tonemap(vec3(W));
	return curr * whiteScale;
}




vec3 T_Unreal(const vec3 x) {
  	return x / (x + 0.155) * 1.019;
}




vec3 Uchimura_Tonemap(const vec3 x, const float P, const float a, const float m, const float l, const float c, const float b) {
	float l0 = ((P - m) * l) / a;
	float L0 = m - m / a;
	float L1 = m + (1.0 - m) / a;
	float S0 = m + l0;
	float S1 = m + a * l0;
	float C2 = (a * P) / (P - S1);
	float CP = -C2 / P;

	vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
	vec3 w2 = vec3(step(m + l0, x));
	vec3 w1 = vec3(1.0 - w0 - w2);

	vec3 T = vec3(m * pow(x / m, vec3(c)) + b);
	vec3 S = vec3(P - (P - S1) * exp(CP * (x - S0)));
	vec3 L = vec3(m + a * (x - m));

	return T * w0 + L * w1 + S * w2;
}

vec3 T_Uchimura(const vec3 x) {
	const float P = 1.0;  
	const float a = 1.0;  
	const float m = 0.22; 
	const float l = 0.4;  
	const float c = 1.33; 
	const float b = 0.0;  

	return Uchimura_Tonemap(x, P, a, m, l, c, b);
}



vec3 T_Lottes(const vec3 x) {
    const float a = 1.6;
    const float d = 0.977;
    const float hdrMax = 8.0;
    const float midIn = 0.18;
    const float midOut = 0.267;

    
    const float b =
        (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const float c =
        (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

    return pow(x, vec3(a)) / (pow(x, vec3(a * d)) * b + c);
}

vec3 ApplyTonemapping(const uint idx, const vec3 color, const float max_lum){
	if(idx == TM_NONE) return color;	
	else if(idx == TM_REINHARD) return T_Reinhard(color);
	else if(idx == TM_REINHARD_EXTENDED) return T_Reinhard(color, max_lum);
	else if(idx == TM_UNCHARTED2) return T_Uncharted2(color);
	else if(idx == TM_UCHIMURA) return T_Uchimura(color);
	else if(idx == TM_LOTTES) return T_Lottes(color);
	else if(idx == TM_FILMIC) return T_Filmic(color);
	else if(idx == TM_ACES) return T_ACES(color);
}

#endif	
#endif 	