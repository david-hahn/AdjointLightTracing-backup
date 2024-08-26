#ifndef  _DEFINES_H_
#define  _DEFINES_H_






#define USE_HSH 1
#define EXACT_HSH 1

#define SH_SMOOTHING (1e-3)

#define USE_GGX 1

#define GV_COMPLETE 0
#define GV_LIGHT_GRAD 1
#define GV_IMAGE_GRAD 2

#define GRAD_VIS_PARTIAL 0

#ifndef USE_SPHERICAL_HARMONICS
#define BOUNCE_SAMPLES 6
#else
#define BOUNCE_SAMPLES 10
#endif


#define BINDLESS_IMAGE_DESC_SET 0
#define BINDLESS_IMAGE_DESC_BINDING 0


#define ADJOINT_DESC_SET 1

#define ADJOINT_DESC_TLAS_BINDING 0
#define ADJOINT_DESC_INFO_BUFFER_BINDING 1
#define ADJOINT_DESC_RADIANCE_BUFFER_BINDING 2
#define ADJOINT_DESC_INDEX_BUFFER_BINDING 3
#define ADJOINT_DESC_VERTEX_BUFFER_BINDING 4
#define ADJOINT_DESC_MATERIAL_BUFFER_BINDING 5
#define ADJOINT_DESC_GEOMETRY_BUFFER_BINDING 6
#define ADJOINT_DESC_AREA_BUFFER_BINDING 7
#define ADJOINT_DESC_LIGHT_BUFFER_BINDING 8
#define ADJOINT_DESC_LIGHT_DERIVATIVES_BUFFER_BINDING 9
#define ADJOINT_DESC_LIGHT_TEXTURE_DERIVATIVES_BUFFER_BINDING 10
#define ADJOINT_DESC_TRIANGLE_BUFFER_BINDING 11


#define OBJ_DESC_RADIANCE_BUFFER_BINDING 0
#define OBJ_DESC_TARGET_RADIANCE_BUFFER_BINDING 1
#define OBJ_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING 2
#define OBJ_DESC_AREA_BUFFER_BINDING 3
#define OBJ_DESC_CHANNEL_WEIGHTS_BUFFER_BINDING 4
#define OBJ_DESC_PHI_BUFFER_BINDING 5
#define OBJ_DESC_VERTEX_COLOR_BUFFER_BINDING 6


#define RASTERIZER_DESC_SET 1

#define RASTERIZER_DESC_GLOBAL_BUFFER_BINDING 0
#define RASTERIZER_DESC_RADIANCE_BUFFER_BINDING 1
#define RASTERIZER_DESC_TARGET_RADIANCE_BUFFER_BINDING 2
#define RASTERIZER_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING 3
#define RASTERIZER_DESC_CHANNEL_WEIGHTS_BUFFER_BINDING 4
#define RASTERIZER_DESC_VTX_COLOR_BUFFER_BINDING 5
#define RASTERIZER_DESC_MATERIAL_BUFFER_BINDING 6
#define RASTERIZER_DESC_AREA_BUFFER_BINDING 7

#define DRAW_DESC_VERTEX_BUFFER_BINDING 0
#define DRAW_DESC_TARGET_RADIANCE_BUFFER_BINDING 1
#define DRAW_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING 2


#define DERIV_VIS_DESC_SET 2
#define DERIV_VIS_DESC_DERIV_IMAGE_BINDING 1
#define DERIV_VIS_DESC_DERIV_ACC_IMAGE_BINDING 2
#define DERIV_VIS_DESC_DERIV_ACC_COUNT_IMAGE_BINDING 3
#define DERIV_VIS_DESC_UBO_BUFFER_BINDING 4
#define DERIV_VIS_DESC_TARGET_BUFFER_BINDING 5
#define DERIV_VIS_DESC_TARGET_RADIANCE_WEIGHTS_BUFFER_BINDING 6
#define DERIV_VIS_DESC_FD_RADIANCE_BUFFER_BINDING 7
#define DERIV_VIS_DESC_FD2_RADIANCE_BUFFER_BINDING 8

#define OBJ_FUNC_WORKGROUP_SIZE 32

#if !defined(HLSL) || !defined(GLSL)
#define IALT_SHADER_DIR "assets/shader/ialt/"
#endif

#ifdef HLSL
#define M_PI 3.14159265358979323846264338327950288f
struct [raypayload] AdjointPayload {
    float2 hitAttribute;
	int instanceIndex;
	int customInstanceID;
	int primitiveIndex;
    int geometryIndex;
	uint hitKind;
	float hit_distance;
};
#elif defined(GLSL)
#define M_PI 3.14159265358979323846264338327950288f
struct RayDesc
{
    vec3    origin;
    float   tmin;
    vec3    direction;
    float   tmax;
    uint    flags;
};
struct AdjointPayload {
    vec2 hitAttribute;
	int instanceIndex;
	int customInstanceID;
	int primitiveIndex;
    int geometryIndex;
	uint hitKind;
	float hit_distance;
};
struct HitDataStruct {
    RayDesc ray;    
    vec3 hitX;
    float texMetallic; 
    vec3 hitN;
    float texRoughness;
    vec3 hitT;
    float eta_int;
    vec3 texColor;
    float eta_ext;
    vec3 texNormal;
    float texTransmission;
    uvec3 idx;
    uvec3 radBuffIdx;
    vec3 bary;
    vec3 p0;
    vec3 p1;
    vec3 p2;
    vec3 n0;
    vec3 n1;
    vec3 n2;
    vec3 t0;
    vec3 t1;
    vec3 t2;
    vec3 v0;
    vec3 v1;
    vec3 v2;
};
struct BounceDataStruct {
    vec3 rayThroughput;
    float rayWeight;
    vec3 wo;
    float inCos;
    uint depth;

};
struct GradDataStruct{
    vec3 dOdPhi_sum;        
    vec3 dOdf_sum;          
    vec3 dBrdfSamplesdP;
    vec3 dBounceBrdfdP;
    vec3 dPhidP;            
    vec3 dPhidN;            
    vec3 dPhidT;            
    vec2 dPhidAio;          
};
struct ParamDerivsDataStruct{
    vec3 position;          
    vec3 normal;            
    vec3 tangent;           
    vec3 color;
    vec2 angles;            
    float intensity;
    vec3 textureColor;
};

struct LightGradDataRGB{
    vec3 dPhidP[3];         
    vec3 dPhidN[3];         
    vec3 dPhidT[3];         
    vec2 dPhidAio[3];       
};

struct ObjGradData{
    vec3 dOdPhi;
};

#endif

#define GLM
#include "../utils/type_conv_def.h"

STRUCT (
    
    
    UINT (light_count)
    UINT (bounces)
    UINT (seed)
    UINT (triangle_count)
    UINT (tri_rays)
    UINT (sam_rays)
,AdjointInfo_s)


STRUCT (
    MAT4    (viewMat)
    MAT4    (inverseViewMat)
    MAT4    (projMat)
    MAT4    (inverseProjMat)
    VEC4    (viewPos)
    VEC4    (viewDir)
    VEC4    (wireframeColor)
    VEC4    (bg_color)
    VEC2    (size)
    UINT    (frameCount)
    BOOL    (shade)
    BOOL    (show_target)
    BOOL    (show_alpha)
    BOOL    (show_adjoint_deriv)
    BOOL    (show_wireframe_overlay)
    FLOAT   (dither_strength)
    UINT    (opti_para_count)
    INT     (light_deriv_idx)
    INT     (param_deriv_idx)
    FLOAT   (adjoint_range)
    BOOL    (log_adjoint_vis)
    FLOAT   (grad_range)
    UINT    (cull_mode)
    BOOL    (log_grad_vis)
    BOOL    (grad_vis_accumulate)
    BOOL    (grad_vis_weights)
    BOOL    (fd_grad_vis)
    FLOAT   (fd_grad_h)
    
    
,GlobalBufferR)

STRUCT (
    MAT4    (modelMatrix)
    VEC3    (positionWS)
    FLOAT   (radius)
    VEC4    (color)
    VEC3    (normal)
    UINT    (vertexOffset)
    VEC3    (originWS)
    BOOL    (softBrush)
    BOOL    (drawAll)
    BOOL    (drawRGB)
    BOOL    (drawALPHA)
    BOOL    (leftMouse)
    BOOL    (setSH)
,DrawBuffer)

STRUCT (
    DVEC4   (dOdColor)
    DVEC4   (dOdP)
    DVEC4   (dOdN)
    DVEC4   (dOdT)
    DOUBLE  (dOdIntensity)
    DOUBLE  (dOdIAngle)
    DOUBLE  (dOdOAngle)
    DOUBLE  (_)
,LightGrads)


#include "../utils/type_conv_undef.h"

#endif /*_DEFINES_H_*/