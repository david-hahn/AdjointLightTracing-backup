#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_atomic_float : require
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_nonuniform_qualifier : require

#define EXEC_MODE (0) 
#include "path_trace_unified.glsl"
