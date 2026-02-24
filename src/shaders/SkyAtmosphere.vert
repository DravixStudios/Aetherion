#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 outDir;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 View;
    mat4 Projection;
} camData;

void main() {
    outDir = inPosition;
    
    mat4 viewProj = camData.View * camData.Projection;
    vec4 pos = viewProj * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
}