#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals; // Unused but vertex layout needs it
layout(location = 2) in vec2 inUVs; // Unused

struct WVP {
    mat4 World;
    mat4 View;
    mat4 Projection;
};

struct ObjectInstanceData {
    uint wvpOffset;
    uint textureIndex;
    uint ormTextureIndex;
    uint emissiveTextureIndex;
};

/* Set 0: Scene data (same as GBufferPass)*/
layout(set = 0, binding = 0) readonly buffer InputInstances {
    ObjectInstanceData instances[];
};

layout(set = 0, binding = 4) readonly buffer WVPBuffer {
    WVP wvpData[];
};

/* Push constants with actual cascade lightViewProj */
layout(push_constant) uniform PushConstants {
    mat4 lightViewProj;
    uint wvpAlignment;
} pc;

void main() {
    ObjectInstanceData instance = instances[gl_InstanceIndex];

    uint wvpIndex = instance.wvpOffset / pc.wvpAlignment;
    WVP wvp = wvpData[wvpIndex];

    vec4 worldPos = wvp.World * vec4(inPosition, 1.0);

    gl_Position = pc.lightViewProj * worldPos;
}
