#version 450

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalImage;

layout(set = 0, binding = 0) uniform samplerCube g_skybox;
layout(set = 0, binding = 1) uniform sampler2DMS g_depth;
layout(set = 1, binding = 0) uniform SkyboxCamera {
    mat4 inverseView;
    mat4 inverseProj;
    vec3 cameraPosition;
} camData;

layout(push_constant) uniform PushConstants {
    uint sampleCount;
} pc;

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy);

    vec2 uv = inUVs;

    vec4 clipPos = vec4(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewPos = camData.inverseProj * clipPos;
    viewPos /= viewPos.w;

    vec4 worldPos = camData.inverseView * viewPos;
    vec3 correctedCameraPos = vec3(camData.cameraPosition.x, camData.cameraPosition.y, camData.cameraPosition.z);
    vec3 dir = normalize(worldPos.xyz - correctedCameraPos);
    dir.x = -dir.x;

    vec3 color = texture(g_skybox, dir).rgb;

    uint skyMask = 0;
    uint totalSkySamples = 0;
    for(uint s = 0; s < pc.sampleCount; ++s) {
        float subpixelDepth = texelFetch(g_depth, p, int(s)).r;
        if(subpixelDepth >= 1.f - 0.00001f) {
            skyMask |= (1u << s);
            totalSkySamples++;
        }
    }

    if(totalSkySamples == 0) {
        discard;
    }

    gl_SampleMask[0] = int(skyMask);
    finalImage = vec4(color, 1.0);
}
