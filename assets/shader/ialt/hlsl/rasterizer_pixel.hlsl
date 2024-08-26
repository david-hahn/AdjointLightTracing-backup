#define HLSL
#include "defines.h"


















[[vk::binding(RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, RASTERIZER_DESC_SET)]] StructuredBuffer<GlobalBufferR> global_buffer;

struct VSOutput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float4 normal : NORMAL0;
    [[vk::location(2)]] float4 tangent : TANGENT0;
    [[vk::location(3)]] float2 uv0 : TEXCOORD0;
    [[vk::location(4)]] float4 color0 : COLOR0;
};

struct PushConsts {
	float4x4 model_matrix;
	int material_idx;
};
[[vk::push_constant]] PushConsts pushConsts;

float4 main(VSOutput input) : SV_TARGET
{
	return float4(input.color0.xyz, 1.0);
}


    
    
    
    
    
    

    
    
    
    
    
    
    

    
	
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    


    
    
    
    
    
    
    

    
    
	
