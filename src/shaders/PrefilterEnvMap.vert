#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outWorldPos;

layout(set = 1, binding = 0) uniform VPBlock {
    mat4 View;
    mat4 Projection;
} vp; 

void main() {
    outWorldPos = inPosition;
    gl_Position = vp.Projection * vp.View * vec4(outWorldPos, 1.0);
}