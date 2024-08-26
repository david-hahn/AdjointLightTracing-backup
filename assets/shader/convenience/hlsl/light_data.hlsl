


#ifndef LIGHT_DATA_HLSL
#define LIGHT_DATA_HLSL
#if defined(CONV_LIGHT_BUFFER_BINDING) && defined(CONV_LIGHT_BUFFER_SET)

#define LIGHT_TYPE_POINT            0x00000001
#define LIGHT_TYPE_SPOT             0x00000002
#define LIGHT_TYPE_DIRECTIONAL      0x00000004
#define LIGHT_TYPE_IES      		0x00000008

#define LIGHT_TYPE_PUNCTUAL         (LIGHT_TYPE_POINT | LIGHT_TYPE_SPOT | LIGHT_TYPE_DIRECTIONAL)
struct Light_s {
	
	float3						    color;
	float							intensity;
	
	
	
	float3						    pos_ws;
	float                           _;
	
	
	
	float3						    n_ws_norm;
	float                           __;
	float3						    t_ws_norm;
	float                           ___;
	
	
	float							range;
	
	float							light_offset;		
	
	float							inner_angle;
	float							outer_angle;
	float							light_angle_scale;
	float							light_angle_offset;
	
	int								texture_index;
	float							min_vertical_angle;
	float							max_vertical_angle;
	float							min_horizontal_angle;
	float							max_horizontal_angle;
	
	uint							type;
};

[[vk::binding(CONV_LIGHT_BUFFER_BINDING, CONV_LIGHT_BUFFER_SET)]] StructuredBuffer<Light_s> light_buffer;


bool isPunctualLight(const uint idx){
	return (light_buffer[idx].type & LIGHT_TYPE_PUNCTUAL) > 0;
}
bool isPointLight(const uint idx){
	return light_buffer[idx].type == LIGHT_TYPE_POINT;
}
bool isSpotLight(const uint idx){
	return light_buffer[idx].type == LIGHT_TYPE_SPOT;
}
bool isDirectionalLight(const uint idx){
	return light_buffer[idx].type == LIGHT_TYPE_DIRECTIONAL;
}
bool isIesLight(const uint idx){
	return light_buffer[idx].type == LIGHT_TYPE_IES;
}

#endif
#endif 	