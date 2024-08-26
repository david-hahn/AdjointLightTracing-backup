


#if defined(CONV_TEXTURE_BINDING) && defined(CONV_TEXTURE_SET)
#ifndef TEXTURE_DATA_HLSL
#define TEXTURE_DATA_HLSL

[[vk::binding(CONV_TEXTURE_BINDING, CONV_TEXTURE_SET)]] Texture2D texture_sampler;

#endif 	
#endif