#version 460 

layout(location = 0) in vec2 in_uv;

layout(push_constant) uniform PushConstant {
    layout(offset = 16) vec3 color;
};

layout(location = 0) out vec4 fragColor;

void main() {
    
    float THICKNESS = 1;
    
    float radius = length(in_uv);
    
    
    float signedDistance = radius - 1.0f;
    
    
    vec2 gradient = vec2(dFdx(signedDistance), dFdy(signedDistance));
    
    float rangeFromLine = abs(signedDistance/length(gradient));
    
    float alpha = clamp(THICKNESS - rangeFromLine, 0.0f, 1.0f);

    
    
    
    
    

    fragColor = vec4(color, alpha);
}