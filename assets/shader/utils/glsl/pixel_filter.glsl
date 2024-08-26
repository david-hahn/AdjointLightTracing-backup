#ifndef GLSL_PIXEL_FILTER
#define GLSL_PIXEL_FILTER

#define PF_BOX 		        0
#define PF_TRIANGLE 	    1
#define PF_GAUSSIAN 	    2
#define PF_BLACKMANN_HARRIS 3
#define PF_MITCHELL         4

#ifdef GLSL
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288f
#endif



float BoxFilter(const vec2 pixel, const vec2 radius) {
    return all(lessThanEqual(abs(pixel), radius)) ? 1.0f : 0.0f;
}


float TriangleFilter(const vec2 pixel, const vec2 radius) {
    return max(0.0f, radius.x - abs(pixel.x)) * max(0.0f, radius.y - abs(pixel.y));
    
    
}


float GaussianFilter(const vec2 pixel, const vec2 radius, const float alpha) {
    const float gExpX = exp(-alpha * radius.x * radius.x);
    const float gExpY = exp(-alpha * radius.y * radius.y);

    const float gaussianX = max(0.0f, exp(-alpha * pixel.x * pixel.x) - gExpX);
    const float gaussianY = max(0.0f, exp(-alpha * pixel.y * pixel.y) - gExpY);
    return gaussianX * gaussianY;
}



float Mitchell1D(float x, const float B, const float C) {
    x = abs(2 * x);
    const float x2 = x * x;
    const float x3 = x2 * x;
    return 1.f / 6.f * x > 1 ? 
        ((-B - 6 * C) * x3 + (6 * B + 30 * C) * x2 + (-12 * B - 48 * C) * x + (8 * B + 24 * C)) :
        ((12 - 9 * B - 6 * C) * x3 + (-18 + 12 * B + 6 * C) * x2 + (6 - 2 * B)) * (1.f / 6.f);
}
float MitchellFilter(const vec2 pixel, const vec2 radius, const float B, const float C) {
    const vec2 invRadius = 1 / radius;
    return Mitchell1D(pixel.x * invRadius.x, B, C) * Mitchell1D(pixel.y * invRadius.y, B, C);
}


float Sinc(float x) {
    x = abs(x);
    if (x < 1e-5)  return 1;
    return sin(M_PI * x) / (M_PI * x);
}
float WindowedSinc(float x, const float radius, const float tau) {
    x = abs(x);
    if (x > radius) return 0;
    const float lanczos = Sinc(x / tau);
    return Sinc(x) * lanczos;
}
float LanczosSincFilter(const vec2 pixel, const vec2 radius, const float tau) {
    return WindowedSinc(pixel.x, radius.x, tau) * WindowedSinc(pixel.y, radius.y, tau);
}



float BlackmanHarrisFilter(const vec2 pixel, const vec2 radius) {
    vec2 v = 2 * M_PI * (abs(pixel) / radius);
    vec2 r = 0.35875f - 0.48829f * cos(v) + 0.14128f * cos(2.0f * v) - 0.01168f * cos(3.0f * v);
    return dot(r, r);
}

float ApplyPixelFilter(const uint idx, const vec2 pixel, const vec2 radius, const vec2 extra) {
    if (idx == PF_BOX) return BoxFilter(pixel, radius);
    else if (idx == PF_TRIANGLE) return TriangleFilter(pixel, radius);
    else if (idx == PF_GAUSSIAN) return GaussianFilter(pixel, radius, extra.x);
    else if (idx == PF_BLACKMANN_HARRIS) return BlackmanHarrisFilter(pixel, radius);
    else if (idx == PF_MITCHELL) return MitchellFilter(pixel, radius, extra.x, extra.y);
    else return 1;
}

#endif
#endif