#define HLSL
#include "defines.h"

[shader("miss")]
void main(inout AdjointPayload ap)
{
	ap.hitAttribute = float2(0, 0);
	ap.instanceIndex = -1;
	ap.customInstanceID = -1;
	ap.primitiveIndex = -1;
	ap.geometryIndex = -1;
	ap.hitKind = 0;
	ap.hit_distance = 0;
}