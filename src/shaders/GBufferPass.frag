#version 450
#extension GL_EXT_nonuniform_qualifier : require // Enable bindless textures

layout(location = 0) in vec3 inNormals;
layout(location = 1) in vec2 inUVs;
layout(location = 2) in vec3 fragPos;

layout(push_constant) uniform PushConstants {
    uint nTextureIndex;
} pc; // Get our push constants

layout(set = 1, binding = 0) uniform sampler2D g_textures[]; // Get our binless textures array.

layout(location = 0) out vec4 outBaseColor;
layout(location = 1) out vec4 outNormals;
layout(location = 2) out vec4 outPosition;

void main() {
    outBaseColor = texture(g_textures[nonuniformEXT(pc.nTextureIndex)], vec2(inUVs.x, 1 - inUVs.y));
    vec3 normals = inNormals * 0.5 + 0.5;
    outNormals = vec4(normals.xyz, 1.0);
    outPosition = vec4(fragPos.xyz, 1.0);
}