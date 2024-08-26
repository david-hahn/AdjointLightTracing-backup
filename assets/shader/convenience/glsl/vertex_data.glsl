




#ifndef VERTEX_DATA_GLSL
#define VERTEX_DATA_GLSL


#if defined(CONV_INDEX_BUFFER_BINDING) && defined(CONV_INDEX_BUFFER_SET)
layout(binding = CONV_INDEX_BUFFER_BINDING, set = CONV_INDEX_BUFFER_SET) buffer index_storage_buffer { uint index_buffer[]; };
#endif


#if defined(CONV_VERTEX_BUFFER_BINDING) && defined(CONV_VERTEX_BUFFER_SET)
struct Vertex_s {
	vec4	position;
	vec4	normal;	
	vec4	tangent;

	vec2	texture_coordinates_0;
	vec2	texture_coordinates_1;
	vec4	color_0;
};
layout(binding = CONV_VERTEX_BUFFER_BINDING, set = CONV_VERTEX_BUFFER_SET) buffer vertex_storage_buffer { Vertex_s vertex_buffer[]; };
#endif

#if defined(CONV_INDEX_BUFFER_BINDING) && defined(CONV_INDEX_BUFFER_SET) && defined(CONV_VERTEX_BUFFER_BINDING) && defined(CONV_VERTEX_BUFFER_SET)
#endif

#endif 	