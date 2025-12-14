#version 450
#extension GL_EXT_nonuniform_qualifier : require // Enable bindless textures
#define INVALID_INDEX 0xFFFFFFFFu

layout(location = 0) in vec3 inNormals;
layout(location = 1) in vec2 inUVs;
layout(location = 2) in vec3 fragPos;

layout(push_constant) uniform PushConstants {
    uint nTextureIndex;
    uint nOrmIndex;
    uint nEmissiveIndex;
} pc; // Get our push constants

layout(set = 1, binding = 0) uniform sampler2D g_textures[]; // Get our bindless textures array.

layout(location = 0) out vec4 outBaseColor;
layout(location = 1) out vec4 outNormals;
layout(location = 2) out vec4 outORM;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out vec4 outPosition;

void main() {
    outBaseColor = texture(g_textures[nonuniformEXT(pc.nTextureIndex)], vec2(inUVs.x, 1 - inUVs.y));
    vec3 normals = inNormals * 0.5 + 0.5;
    outNormals = vec4(normals.xyz, 1.0);
    outORM =  texture(g_textures[nonuniformEXT(pc.nOrmIndex)], vec2(inUVs.x, 1 - inUVs.y));
    if(pc.nEmissiveIndex != INVALID_INDEX) {
        outEmissive =  texture(g_textures[nonuniformEXT(pc.nEmissiveIndex)], vec2(inUVs.x, 1 - inUVs.y));
    } else {
        outEmissive = vec4(0.0, 0.0, 0.0, 0.0);
    }
    outPosition = vec4(fragPos.xyz, 1.0);
}