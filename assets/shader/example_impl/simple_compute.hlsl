
[[vk::binding(0,0)]] RWStructuredBuffer<uint> data;

[[vk::constant_id(0)]] const int MULTIPLIER = 0;

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if(DTid.x == 0) {
        uint numStructs;
        uint stride;
        data.GetDimensions(numStructs, stride);
        printf("Struct count: %d, Struct size: %d byte", numStructs, stride);
        printf("Total buffer size: %d byte", numStructs * stride);
    }

    uint2 ipos = uint2(DTid.xy);
    printf("%d", data[ipos.x]);  
    
    data[ipos.x] = data[ipos.x] * MULTIPLIER;
}

