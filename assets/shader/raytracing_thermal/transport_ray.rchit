#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "payload.glsl"
layout(location = 0) rayPayloadInEXT RayPayload rp;

hitAttributeEXT vec2 hitAttribute;

void main() 
{
	rp.hitAttribute = hitAttribute;
  	rp.instanceID = gl_InstanceID;
	rp.primitiveID = gl_PrimitiveID;
	rp.geometryIndex = gl_GeometryIndexEXT;
	rp.hit_distance = gl_RayTmaxEXT;
}