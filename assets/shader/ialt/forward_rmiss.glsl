#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_tracing : require
#define GLSL
#include "defines.h"

layout(location = 0) rayPayloadInEXT AdjointPayload ap;

void main()
{
	ap.hitAttribute = vec2(0, 0);
	ap.instanceIndex = -1;
	ap.customInstanceID = -1;
	ap.primitiveIndex = -1;
	ap.geometryIndex = -1;
	ap.hitKind = 0;
	ap.hit_distance = 0;
}