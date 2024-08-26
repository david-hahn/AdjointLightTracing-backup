#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#define GLSL
#include "defines.h"
layout(location = 0) rayPayloadInEXT AdjointPayload ap;

hitAttributeEXT vec2 hitAttributes;

void main()
{
    ap.hitAttribute = hitAttributes;
  	ap.instanceIndex = gl_InstanceID;
	ap.customInstanceID = gl_InstanceCustomIndexEXT;
	ap.primitiveIndex = gl_PrimitiveID;
	ap.geometryIndex = gl_GeometryIndexEXT;
	ap.hitKind = gl_HitKindEXT;
	ap.hit_distance = gl_RayTmaxEXT;
}