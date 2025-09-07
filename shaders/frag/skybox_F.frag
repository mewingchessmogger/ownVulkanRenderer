#version 450

layout(set = 0, binding = 0) uniform samplerCube skyboxTex;

// Push constants from C++
layout(push_constant) uniform CameraData {
    mat4 invView;
    mat4 invProj;
} cam;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
    // Build NDC coords in range [-1,1]
    vec4 ndc = vec4(uv, 1.0, 1.0);

    // Back-project into view space
    vec4 viewPos = cam.invProj * ndc;
    viewPos /= viewPos.w; // homogenize

    // Transform into world space
    vec4 worldPos = cam.invView * viewPos;
    vec3 dir = normalize(vec3(worldPos.x,-worldPos.y,worldPos.z));
    dir.y = -dir.y;
    // Sample cubemap with direction
    outColor = texture(skyboxTex, dir);
}
