#ifndef  DEFINES_H
#define  DEFINES_H


#define BINDLESS_IMAGE_DESC_SET 0
#define BINDLESS_IMAGE_DESC_BINDING 0


#define GLOBAL_DESC_SET 1
#define GLOBAL_DESC_UBO_BINDING                 0
#define GLOBAL_DESC_INDEX_BUFFER_BINDING        1
#define GLOBAL_DESC_VERTEX_BUFFER_BINDING       2
#define GLOBAL_DESC_GEOMETRY_DATA_BINDING       3
#define GLOBAL_DESC_MATERIAL_BUFFER_BINDING     4
#define GLOBAL_DESC_AS_BINDING                  5

#define GLSL_GLOBAL_LIGHT_DATA_BINDING          6
#define GLSL_GLOBAL_RT_OUT_IMAGE_BINDING        8
#define GLSL_GLOBAL_RT_ACC_IMAGE_BINDING        9
#define GLSL_GLOBAL_RT_ACC_C_IMAGE_BINDING      10
#define GLSL_GLOBAL_DEBUG_IMAGE_BINDING         11


#define BINDLESS_CUBE_IMAGE_DESC_SET 2
#define BINDLESS_CUBE_IMAGE_DESC_BINDING 0

#define HD_POINT_LIGHT (1)
#define HD_SURFACE_LIGHT (2)

#define GLM
#include "../utils/type_conv_def.h"


STRUCT (
    MAT4    (viewMat)
    MAT4    (inverseViewMat)
    MAT4    (projMat)
    MAT4    (inverseProjMat)
    VEC4    (viewPos)
    VEC4    (viewDir)
    VEC2    (size)
    FLOAT   (frameIndex)
    UINT    (accumulatedFrames)

    VEC4    (bg)

    BOOL    (accumulate)
    UINT    (pixelSamplesPerFrame)
    INT     (max_depth)
    BOOL    (shade)

    VEC2    (debugPixelPosition)
    FLOAT   (dither_strength)
    FLOAT   (exposure_film)

    FLOAT   (fr_tmin)
    FLOAT   (fr_tmax)
    FLOAT   (br_tmin)
    FLOAT   (br_tmax)
    FLOAT   (sr_tmin)
    FLOAT   (sr_tmax_offset)
    UINT    (light_count)
    UINT    (sampling_strategy)

    VEC2    (pixel_filter_extra)
    UINT    (pixel_filter_type)
    FLOAT   (pixel_filter_width)

    UINT    (tone_mapping_type)
    UINT    (cull_mode)
    FLOAT   (exposure_tm)
    FLOAT   (gamma_tm)

    BOOL    (env_shade)
    INT     (rrpt) 
    FLOAT   (filter_glossy)
    FLOAT   (clamp_direct)
    FLOAT   (clamp_indirect)
    BOOL    (light_geometry)
,GlobalUbo_s)




#endif /*DEFINES_H*/