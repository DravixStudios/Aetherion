#version 450

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalImage;

layout(set = 0, binding = 0) uniform sampler2D g_gbuffers[3];

void main() {
    finalImage = texture(g_gbuffers[0], vec2(inUVs.x, 1.0 - inUVs.y));
}