#version 450
#extension GL_EXT_nonuniform_qualifier : require // Enable bindless textures
#define INVALID_INDEX 0xFFFFFFFFu

layout(location = 0) in vec3 inNormals;
layout(location = 1) in vec2 inUVs;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in flat uint inTextureIndex;
layout(location = 4) in flat uint inOrmTextureIndex;
layout(location = 5) in flat uint inEmissiveTextureIndex;

layout(set = 1, binding = 0) uniform sampler2D g_textures[]; // Get our bindless textures array.

layout(location = 0) out vec4 outBaseColor;
layout(location = 1) out vec4 outNormals;
layout(location = 2) out vec4 outORM;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out vec4 outPosition;

void main() {
    outBaseColor = texture(g_textures[nonuniformEXT(inTextureIndex)], vec2(inUVs.x, inUVs.y));
    outBaseColor.rgb = pow(outBaseColor.rgb, vec3(2.2));
    vec3 normals = inNormals * 0.5 + 0.5;
    outNormals = vec4(normals.xyz, 1.0);
    outORM =  texture(g_textures[nonuniformEXT(inOrmTextureIndex)], vec2(inUVs.x, inUVs.y));
    if(inEmissiveTextureIndex != INVALID_INDEX) {
        outEmissive =  texture(g_textures[nonuniformEXT(inEmissiveTextureIndex)], vec2(inUVs.x, inUVs.y));
        outEmissive.rgb = pow(outEmissive.rgb, vec3(2.2));
    } else {
        outEmissive = vec4(0.0, 0.0, 0.0, 0.0);
    }
    outPosition = vec4(fragPos.xyz, 1.0);
}