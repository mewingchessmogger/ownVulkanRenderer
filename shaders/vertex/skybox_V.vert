#version 450
layout(push_constant) uniform CameraData {
    mat4 invView;  // 64 bytes
    mat4 invProj;  // 64 bytes
} cam;
// Hardcoded fullscreen triangle positions
const vec2 verts[3] = vec2[](
    vec2(-1.0, -1.0), // bottom-left
 vec2( 3.0, -1.0), // <— was last
    vec2(-1.0,  3.0)  // <— was middle
);

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = vec4(verts[gl_VertexIndex], 0.0, 1.0);
    uv = verts[gl_VertexIndex]; // can be used to reconstruct NDC
}