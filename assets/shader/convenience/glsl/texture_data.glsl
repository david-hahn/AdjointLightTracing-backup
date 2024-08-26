


#if defined(CONV_TEXTURE_BINDING) && defined(CONV_TEXTURE_SET)
#ifndef TEXTURE_DATA_GLSL
#define TEXTURE_DATA_GLSL

layout(binding = CONV_TEXTURE_BINDING, set = CONV_TEXTURE_SET) uniform sampler2D texture_sampler[];

#endif 	
#endif