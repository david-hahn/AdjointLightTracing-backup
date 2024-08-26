#ifndef  _DEFINES_H_
#define  _DEFINES_H_



#define USE_QUANTIZED_RAY_TRANSPORT 1;





#define MAX_NUM_POINT_LIGHTS 8
#define MAX_NUM_DIRECTIONAL_LIGHTS 8
#define MAX_NUM_SPOT_LIGHTS 8

#define GLSL_GLOBAL_DESC_SET                    0
#define GLSL_GLOBAL_IMAGE_DESC_SET              1

#define GLSL_GLOBAL_IMAGE_BINDING               0

#define GLSL_GLOBAL_UBO_BINDING                 0
#define GLSL_GLOBAL_INSTANCE_DATA_BINDING       1
#define GLSL_GLOBAL_GEOMETRY_DATA_BINDING       2
#define GLSL_GLOBAL_INDEX_BUFFER_BINDING        3
#define GLSL_GLOBAL_VERTEX_BUFFER_BINDING       4
#define GLSL_GLOBAL_LIGHT_DATA_BINDING          5
#define GLSL_GLOBAL_AS_BINDING                  6
#define GLSL_GLOBAL_NOISE_IMAGE_BINDING         7
#define GLSL_GLOBAL_OUT_IMAGE_BINDING           8
#define GLSL_GLOBAL_TRANSPORT_DATA_BINDING      9
#define GLSL_GLOBAL_AUX_UBO_BINDING             10
#define GLSL_GLOBAL_KELVIN_DATA_BINDING         11
#define GLSL_GLOBAL_AREA_DATA_BINDING           12
#define GLSL_GLOBAL_EMISSION_DATA_BINDING       13
#define GLSL_GLOBAL_ABSORPTION_DATA_BINDING     14

#ifdef GLSL
#define M_PI 3.14159265358979323846264338327950288f
#else

#endif


#ifdef GLSL
    #define STRUCT(content, name) struct name { content };
    #define BOOL(n) bool n;
    #define INT(n) int n;
    #define UINT(n) uint n;
    #define FLOAT(n) float n;
    
    #define VEC2(n) vec2 n;
    #define VEC3(n) vec3 n;
    #define VEC4(n) vec4 n;
    #define UVEC2(n) uvec2 n;
    #define UVEC3(n) uvec3 n;
    #define UVEC4(n) uvec4 n;
    
    #define MAT4(n) mat4 n;
    #define MAT4X3(n) mat4x3 n;
#else
    #define STRUCT(content, name) typedef struct { content } name;
    #define BOOL(n) unsigned int n;
    #define INT(n) int n;
    #define UINT(n) unsigned int n;
    #define FLOAT(n) float n;
    
    #define VEC2(n) float n[2];
    #define VEC3(n) float n[3];
    #define VEC4(n) float n[4];
    #define UVEC2(n) unsigned int[2] n;
    #define UVEC3(n) unsigned int[3] n;
    #define UVEC4(n) unsigned int[4] n;
    
    #define MAT4(n) float n[16];
    #define MAT4X3(n) float n[12];
#endif


STRUCT (
    MAT4    (viewMat)
    MAT4    (inverseViewMat)
    MAT4    (projMat)
    MAT4    (inverseProjMat)
    VEC4    (viewPos)
    VEC4    (viewDir)
, GlobalUbo)


STRUCT(
    INT     (instanceCount)
    UINT    (vertexCount)
    UINT    (sourceVertexInd)
    FLOAT   (displayScale)
    UINT    (visualization)
    FLOAT   (kelvinNormFactor)
    UINT    (rayCount)
    UINT    (batchSeed)
    FLOAT   (clipDistance)
    FLOAT   (boundingSphereRadius)
    FLOAT   (align0)
    FLOAT   (align1)
    VEC3    (boundingSphereCenter) 
    FLOAT   (align2)
, AuxiliaryUbo)


STRUCT(
    MAT4    (model_matrix)
    MAT4    (trp_inv_model_matrix)
    INT     (geometry_buffer_offset)
    UINT    (geometryCount)
    FLOAT   (diffuseReflectance)
    FLOAT   (specularReflectance)
    INT     (diffuseEmission)
    FLOAT   (absorption)
    INT     (traceable)
    BOOL    (traceBoundingCone)
, InstanceSSBO)

STRUCT(
    INT     (index_buffer_offset)
    INT     (vertex_buffer_offset)
    BOOL    (has_indices)
    INT     (baseColorTexIdx)
    VEC4    (baseColorFactor)
    FLOAT   (alphaCutoff)
    UINT    (triangleCount)
    INT     (aligned1)
    INT     (aligned2)
, GeometrySSBO)


STRUCT(
    VEC4(factor)
, TransportFactor)



#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2
STRUCT(
    VEC3    (color)
    FLOAT   (intensity)
    VEC3    (position)
    FLOAT   (range)
    VEC3    (direction)
    FLOAT   (lightAngleScale)
    VEC2    (alligne)
    INT     (type)
    FLOAT   (lightAngleOffset)
, PunctualLight)

STRUCT(
    PunctualLight       punctual_lights[8];
    INT                 (punctual_light_count)
, LightsSSBO)


#undef STRUCT
#undef BOOL
#undef INT
#undef UINT
#undef FLOAT

#undef VEC2
#undef VEC3
#undef VEC4
#undef UVEC2
#undef UVEC3
#undef UVEC4

#undef MAT4
#undef MAT4X3

#endif /*_DEFINES_H_*/