#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 outNormals;
layout(location = 2) in vec2 outUVs;

layout(push_constant) uniform PushConstants {
    uint nTextureIndex;
} pc;
layout(set = 1, binding = 0 ) uniform sampler2D g_textures[];

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(g_textures[nonuniformEXT(pc.nTextureIndex)], vec2(outUVs.x, 1 - outUVs.y));
}