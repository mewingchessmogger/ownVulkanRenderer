#version 450
struct Lights {
    vec4 position;
    vec4 color;
    vec4 direction;
    vec4 padding;
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


//VBO DONT CARE ABOUT 16 BYTE ALIGNMENET UBO AND PC:S DO!!!!!!!!!!
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;


layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 fragPos;
layout(location = 2) out vec3 vNormal;


void main() {
    

    gl_Position = ubo.proj *ubo.view *pc.model *vec4(pos, 1.0);
    uv = texCoord;
    fragPos= pc.model *vec4(pos,1.0);

    mat3 normalMat = mat3(transpose(inverse(pc.model)));
    vNormal = normalize(normalMat * normal);


}