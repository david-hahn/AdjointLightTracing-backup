

#ifndef _COLORCONV_
#define _COLORCONV_

const float eps = 216.f/24389.f;
const float kap = 24389.f/27.f;
const vec3  d65_2deg = vec3(0.95047,1.00000,1.08883);
const mat3 rgb2xyz_mat = 
    mat3(0.4124564,0.3575761,0.1804375,
         0.2126729,0.7151522,0.0721750,
         0.0193339,0.1191920,0.9503041);
const mat3 xyz2rgb_mat =
	mat3( 3.2404542, -1.5371385, -0.4985314,
		 -0.9692660,  1.8760108,  0.0415560,
	      0.0556434, -0.2040259,  1.0572252);

float compand(float f){
    return f>0.04045? pow(((f+0.055f)/1.055f),2.4f): f/12.92f;
}
float invcompand(float f){
    return pow(f,1.f/2.4f);
}


vec3 rgb2xyz(vec3 rgb){
	rgb.r = compand(rgb.r);
	rgb.g = compand(rgb.g);
	rgb.b = compand(rgb.b);
    return rgb*rgb2xyz_mat;
}
vec3 xyz2rgb(vec3 xyz){
    xyz *= xyz2rgb_mat;
	xyz.x = invcompand(xyz.x);
	xyz.y = invcompand(xyz.y);
	xyz.z = invcompand(xyz.z);
    return xyz;
}

vec3 xyz2lab(vec3 xyz){
    vec3 f;
    f.x = xyz.x>eps? pow(xyz.x,1.f/3.f) : (kap*xyz.x+16.f)/116.f;
    f.y = xyz.y>eps? pow(xyz.y,1.f/3.f) : (kap*xyz.y+16.f)/116.f;
    f.z = xyz.z>eps? pow(xyz.z,1.f/3.f) : (kap*xyz.z+16.f)/116.f;
    return vec3(116.f* f.y-16.f,
                500.f*(f.x-f.y),
                200.f*(f.y-f.z))/vec3(100)/d65_2deg;
}

vec3 lab2xyz(vec3 lab){
    lab *= 100.f;
    lab *= d65_2deg;
    float fy = (lab.x+16.f)/116.f;
    float fx = lab.y/500.f + fy;
    float fz = fy - (lab.z/200.f);
    float fx3 = pow(fx,3.f);
    float fz3 = pow(fz,3.f);
    return vec3(
    	fx3>eps? fx3: (116.f*fx-16.f)/kap,
        lab.x > (kap*eps)? pow((lab.x+16.f)/116.f,3.f): lab.x/kap,
        fz3>eps? fz3: (116.f*fz-16.f)/kap);
}

#endif
