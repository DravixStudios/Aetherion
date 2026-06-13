#version 450
#extension GL_EXT_nonuniform_qualifier : require // Enable bindless textures
#define INVALID_INDEX 0xFFFFFFFFu

struct MaterialInstanceData {
    uint albedoIndex;
    uint ormIndex;
    uint emissiveIndex;
    uint normalIndex;

    vec4 albedoColor;
    vec4 emissiveColor;
    float ao;
    float roughness;
    float metallic; 

    uint materialFlags;
};

const uint MATERIAL_FLAG_NONE = 0u;
const uint MATERIAL_FLAG_ALBEDO = 1u;
const uint MATERIAL_FLAG_ORM = 1 << 1;
const uint MATERIAL_FLAG_EMISSIVE = 1 << 2;
const uint MATERIAL_FLAG_NORMAL = 1 << 3;

layout(set = 0, binding = 1) readonly buffer MaterialInstances {
    MaterialInstanceData materials[];
};

layout(location = 0) in vec3 inNormals;
layout(location = 1) in vec2 inUVs;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in flat uint inMaterialIndex;

layout(set = 1, binding = 0) uniform sampler2D g_textures[]; // Get our bindless textures array.

layout(location = 0) out vec4 outAlbedoColor;
layout(location = 1) out vec4 outNormals;
layout(location = 2) out vec4 outORM;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out vec4 outPosition;

bool HasFlag(uint flags, uint flag) {
    if((flags & flag) != 0u) {
        return true;
    }

    return false;
}

void main() {
    MaterialInstanceData material = materials[inMaterialIndex];
    uint materialFlags = material.materialFlags;

    vec4 albedoColor = vec4(material.albedoColor);
    vec4 ormColor = vec4(material.ao, material.roughness, material.metallic, 1.0);
    vec4 emissiveColor = vec4(material.emissiveColor);
    vec3 normals = inNormals;

    /* Albedo */ 
    if(HasFlag(materialFlags, MATERIAL_FLAG_ALBEDO)) {
        albedoColor = texture(g_textures[nonuniformEXT(material.albedoIndex)], inUVs);
        albedoColor.rgb = pow(albedoColor.rgb, vec3(2.2));
    }

    outAlbedoColor = albedoColor;

    /* Normals */
    vec3 N = normalize(inNormals);
    normals = N;
    if(HasFlag(materialFlags, MATERIAL_FLAG_NORMAL)) {
        vec3 tangentNormals = texture(
            g_textures[nonuniformEXT(material.normalIndex)],
            inUVs
        ).xyz * 2.0 - 1.0;

        vec3 dp1 = dFdx(fragPos);
        vec3 dp2 = dFdy(fragPos);

        vec2 duv1 = dFdx(inUVs);
        vec2 duv2 = dFdy(inUVs);

        vec3 T = normalize(dp1 * duv2.y - dp2 * duv1.y);
        vec3 B = normalize(-dp1 * duv2.x + dp2 * duv1.x);

        mat3 TBN = mat3(T, B, N);

        normals = normalize(TBN * tangentNormals);
    }

    outNormals = vec4(normals * 0.5 + 0.5, 1.0);

    /* ORM */
    if(HasFlag(materialFlags, MATERIAL_FLAG_ORM)) {
        ormColor = texture(g_textures[nonuniformEXT(material.ormIndex)], inUVs);
    }

    outORM = ormColor;

    /* Emissive */
    if(HasFlag(materialFlags, MATERIAL_FLAG_EMISSIVE)) {
        emissiveColor = texture(g_textures[nonuniformEXT(material.emissiveIndex)], inUVs);
        emissiveColor.rgb = pow(emissiveColor.rgb, vec3(2.2));
    }

    outEmissive = emissiveColor;

    outPosition = vec4(fragPos.xyz, 1.0);
}