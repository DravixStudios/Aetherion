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

    vec4 clipPos = vec4(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = pc.inverseProjection * clipPos;
    viewPos /= viewPos.w;

    vec4 worldPos = pc.inverseView * viewPos;
    vec3 correctedCameraPos = vec3(pc.cameraPosition.x, pc.cameraPosition.y, -pc.cameraPosition.z);
    vec3 dir = normalize(worldPos.xyz - correctedCameraPos);
    
    float sceneDepth = texture(g_depth, vec2(inUVs.x, 1 - inUVs.y)).r; 
    if(sceneDepth < 1.0 - 0.00001) discard;

    vec3 color = texture(g_skybox, dir).rgb;
    // color = color / (color + 1.0);
    finalImage = vec4(color, 1.0);
}