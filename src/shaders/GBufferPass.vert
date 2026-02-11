#version 450

/* Input locations */
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec2 inUVs;

struct WVP {
    mat4 World;
    mat4 View;
    mat4 Projection;
};

struct ObjectInstanceData {
    uint wvpOffset; // WVP Ring buffer dynamic offset
    uint textureIndex;
    uint ormTextureIndex;
    uint emissiveTextureIndex;
};
 
/* Bind set 0 (culling) */
layout(set = 0, binding = 0) readonly buffer InputInstances {
    ObjectInstanceData instances[];
};

layout(set = 0, binding = 4) readonly buffer WVPBuffer {
    WVP wvpData[];
};

layout(push_constant) uniform PushConstants {
    uint wvpAlignment;
} pc;

/* Output locations */
layout(location = 0) out vec3 outNormals;
layout(location = 1) out vec2 outUVs;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out flat uint outTextureIndex;
layout(location = 4) out flat uint outOrmTextureIndex;
layout(location = 5) out flat uint outEmissiveTextureIndex;

void main() {
    /* Use gl_InstanceIndex that comes from the indirect draw */ 
    ObjectInstanceData instance = instances[gl_InstanceIndex];

    uint wvpIndex = instance.wvpOffset / pc.wvpAlignment;
    WVP wvp = wvpData[wvpIndex];

    /* Multiply our vertex position by our wold matrix */ 
    vec4 worldPos = wvp.World * vec4(inPosition, 1.0);

    /* Multiply our Porjection * View * Vertex World position */ 
    gl_Position = wvp.Projection * wvp.View * worldPos;

    /* Fragment position = Vertex world position */ 
    fragPos = worldPos.xyz;

    /* Invert our world matrix and transpose it */ 
    mat3 normalMatrix = transpose(inverse(mat3(wvp.World)));
    outNormals = normalize(normalMatrix * inNormals); // Normal matrix * input normals. Then normalize normals
    
    outUVs = inUVs;

    /* Send the indices to the fragment shader  for bindless textures */
    outTextureIndex = instance.textureIndex;
    outOrmTextureIndex = instance.ormTextureIndex;
    outEmissiveTextureIndex = instance.emissiveTextureIndex;
}