#ifndef  _OPTI_PARA_BUFFER_H_
#define  _OPTI_PARA_BUFFER_H_

#define OPTI_PARA_POS_X            0x00000001u
#define OPTI_PARA_POS_Y            0x00000002u
#define OPTI_PARA_POS_Z            0x00000004u
#define OPTI_PARA_ILLUMINATION     0x00000008u

#define GLM
#include "../helper/type_conv_def.h"

STRUCT (
    UINT    (para_type_flags) /* OPTI_PARA_ */
    UINT    (light_index)
,OptiPara_s)

#include "../helper/type_conv_undef.h"

#if defined(GLSL) && defined(OPTI_PARA_BUFFER_BINDING) && defined(OPTI_PARA_BUFFER_SET)
layout(binding = OPTI_PARA_BUFFER_BINDING, set = OPTI_PARA_BUFFER_SET) buffer opti_para_storage_buffer { OptiPara_s opti_para_buffer[]; };
#endif

#endif /*_OPTI_PARA_BUFFER_H_*/