#version 450
struct Lights {
    vec4 position;
    vec4 color;
    vec4 dir;
    
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    Lights lights[8];
    int currLightCount;
} ubo;

layout(push_constant) uniform PushData {
    mat4 model;
    vec3 color; 
    float padding;
    int useTexture;
} pc;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 fragPos;
layout(location = 2) in vec3 vNormal;


layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {

    vec4 tex =  texture(texSampler, vec2(uv.x,uv.y));
    vec4 color = (pc.useTexture == 1) ? tex : vec4(pc.color.xyz,1.0f); 

    float sun_light= dot(normalize(ubo.lights[0].dir.xyz), normalize(vNormal.xyz)); 
    sun_light = max(sun_light,0.0f);//prevent negative vals

    vec3 ambient = 0.06 * color.rgb;
    vec3 diffuse = color.rgb * sun_light * 0.5;

    vec3 lit = ambient + diffuse;
    outColor = vec4(lit, color.a);

    
}