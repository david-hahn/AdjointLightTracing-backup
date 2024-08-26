#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_tracing : require

#include "payload.glsl"
layout(location = 0) rayPayloadInEXT RayPayload rp;

void main() 
{
	rp.hitAttribute = vec2(0);
  	rp.instanceID = -1;
	rp.primitiveID = -1;
	rp.geometryIndex = -1;
	rp.hit_distance = 0;
}
