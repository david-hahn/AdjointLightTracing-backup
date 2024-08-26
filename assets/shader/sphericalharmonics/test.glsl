#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : enable


struct metaData{
    vec3 dir;
	int order;

	int sh_offset;
	int zh_offset;
};

layout(binding = 0, set = 0, std430) buffer metaBuffer { metaData meta; };
layout(binding = 1, set = 0, std430) buffer dataBuffer { float sh_data[];  };

#define SH_DATA_BUFFER_ sh_data
#include "sphericalharmonics.glsl" 

void main() {
    debugPrintfEXT("%.7f %d", data[0], meta.order);
    float phi,theta;
    toSphericalCoords(meta.dir, phi, theta);
    debugPrintfEXT("Y_0^0 = %.7f", evalSH(0, 0, meta.dir));
    debugPrintfEXT("dir = (%.3f %.3f %.3f)", meta.dir.x, meta.dir[1], meta.dir.z);
    debugPrintfEXT("phi = %.5f  theta = %.5f", phi,theta);

    float sh = evalSHSum(meta.order, meta.sh_offset, phi, theta);
    debugPrintfEXT("SH(phi,theta) = %.5f", sh);

    rotateZHtoSH(meta.order, meta.sh_offset, meta.zh_offset, phi, theta);
    data[meta.zh_offset] = sh;
}
