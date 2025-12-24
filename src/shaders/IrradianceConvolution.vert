#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 View;
    mat4 Projection;
}

void main() {
    outWorldPos = inPosition;
    gl_Position = pc.Projection * pc.View * vec4(inPosition, 1.0);
}