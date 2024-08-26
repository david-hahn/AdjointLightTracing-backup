#define HLSL
#include "defines.h"











struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float4 normal : NORMAL0;
    [[vk::location(2)]] float4 tangent : TANGENT0;
    [[vk::location(3)]] float2 uv0 : TEXCOORD0;
    [[vk::location(4)]] float2 uv1 : TEXCOORD1;
    [[vk::location(5)]] float4 color0 : COLOR0;
};

struct PushConsts {
	float4x4 model_matrix;
	int material_idx;
};
[[vk::push_constant]] PushConsts pcs;

[[vk::binding(RASTERIZER_DESC_GLOBAL_BUFFER_BINDING, RASTERIZER_DESC_SET)]] ConstantBuffer<GlobalBufferR> global_buffer;

struct VSOutput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float4 normal : NORMAL0;
    [[vk::location(2)]] float4 tangent : TANGENT0;
    [[vk::location(3)]] float2 uv0 : TEXCOORD0;
    [[vk::location(4)]] float4 color0 : COLOR0;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(global_buffer.projMat, mul(global_buffer.viewMat, mul(pcs.model_matrix, float4(input.position.xyz, 1.0))));

    

    output.color0 = input.color0;
    output.uv0 = input.uv0;

    float4x4 mat = transpose(inverse(pcs.model_matrix));
    
    
    return output;
}