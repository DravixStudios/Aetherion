#version 450

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalImage;

layout(push_constant) uniform PushConstants {
    mat4 inverseView;
    mat4 inverseProjection;
    vec3 cameraPosition;
} pc;

layout(set = 0, binding = 0) uniform samplerCube g_skybox;
layout(set = 0, binding = 1) uniform sampler2D g_depth;

void main() {
    vec2 uv = inUVs;
    float depth = 1.0; // Skybox "infinite"

    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = pc.inverseProjection * clipPos;
    viewPos /= viewPos.w;

    vec4 worldPos = pc.inverseView * viewPos;
    vec3 dir = normalize(worldPos.xyz - pc.cameraPosition);

    float sceneDepth = texture(g_depth, vec2(uv.x, 1.0 - uv.y)).r;
    if(sceneDepth < 1) discard;

    vec3 color = texture(g_skybox, dir).rgb;
    finalImage = vec4(color, 1.0);
}