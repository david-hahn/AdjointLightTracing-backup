#define HLSL
#include "defines.h"

[shader("closesthit")]
void main(inout AdjointPayload ap, in float2 hitAttributes)
{
    ap.hitAttribute = hitAttributes;
  	ap.instanceIndex = InstanceIndex();
	ap.customInstanceID = InstanceID();
	ap.primitiveIndex = PrimitiveIndex();
	ap.geometryIndex = GeometryIndex();
	ap.hitKind = HitKind();
	ap.hit_distance = RayTCurrent();
}