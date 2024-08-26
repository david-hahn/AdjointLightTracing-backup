#ifndef  _DEFINES_H_
#define  _DEFINES_H_

#define MAX_NUM_POINT_LIGHTS 8
#define MAX_NUM_DIRECTIONAL_LIGHTS 8
#define MAX_NUM_SPOT_LIGHTS 8


#define BINDLESS_IMAGE_DESC_SET 0
#define BINDLESS_IMAGE_DESC_BINDING 0


#define GLOBAL_DESC_SET 1
#define GLOBAL_DESC_UBO_BINDING 0
#define GLOBAL_DESC_MATERIAL_BUFFER_BINDING 1
#define GLOBAL_DESC_LIGHT_BUFFER_BINDING 2

#ifdef GLSL
#define M_PI 3.14159265358979323846264338327950288f
#else

#endif

#define GLM
#include "../utils/type_conv_def.h"

#define DISPLAY_MODE_DEFAULT        0
#define DISPLAY_MODE_VERTEX_COLOR   1
#define DISPLAY_MODE_METALLIC       2
#define DISPLAY_MODE_ROUGHNESS      3
#define DISPLAY_MODE_NORMAL_MAP     4
#define DISPLAY_MODE_OCCLUSION      5
#define DISPLAY_MODE_LIGHT_MAP      6


STRUCT (
    MAT4    (viewMat)
    MAT4    (inverseViewMat)
    MAT4    (projMat)
    MAT4    (inverseProjMat)
    VEC4    (viewPos)
    VEC4    (viewDir)
    VEC2    (size)
    FLOAT   (frameCount)
    BOOL    (shade)
    UINT    (display_mode)
    UINT    (rgb_or_alpha)
    FLOAT   (dither_strength)
    UINT    (light_count)
    VEC4    (normal_color)
    VEC4    (tangent_color)
    VEC4    (bitangent_color)
    BOOL    (wireframe)
    BOOL    (useLightMaps)
    BOOL    (mulVertexColors)
,GlobalUbo)

#include "../utils/type_conv_undef.h"

#endif /*_DEFINES_H_*/