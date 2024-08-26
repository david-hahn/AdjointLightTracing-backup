#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : enable


struct metaData{
	uint samples;
	float roughness;
};

layout(binding = 0, set = 0, std430) buffer metaBuffer { metaData meta; };
layout(binding = 1, set = 0, std430) buffer dataBuffer { float data[];  };


#include "../utils/glsl/ray_tracing_utils.glsl"
#include "../utils/glsl/rendering_utils.glsl"
#include "../utils/glsl/bxdfs/metallic_roughness.glsl"

float brdf_eval(const vec3 V, const vec3 L, const vec3 N, float roughness){
	const vec3 c = vec3(1.0,0.0,0.0);
	vec3 brdf_value = metalroughness_eval(V, L, N, c, 0.5, roughness, 0.04f /*default f0*/);
	return brdf_value.x;
}

void main() {
	
	uint seed=0; float ggxVal;
	vec3 unitZ = {0.0, 0.0, 1.0};
	for(int k=0; k<meta.samples; ++k){
		vec2 uv = vec2(randomFloat(seed),randomFloat(seed));
		vec3 v = sampleUnitSphereUniform(uv);
		float val = brdf_eval(v, unitZ, unitZ, meta.roughness);
		
		data[4*k  ]=v[0];
		data[4*k+1]=v[1];
		data[4*k+2]=v[2];
		data[4*k+3]=val;
	}
}