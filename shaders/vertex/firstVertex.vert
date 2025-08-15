#version 450
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushData {
    mat4 model;
} pc;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;


void main() {
    gl_Position = ubo.proj *ubo.view *pc.model *vec4(pos, 1.0);
    fragColor = color;
    fragTexCoord = texCoord;
}