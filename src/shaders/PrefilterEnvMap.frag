#version 450
#define PI 3.14159265359

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform samplerCube g_environmentMap;

layout(push_constant) uniform PushConstants {
    float Roughness;
    uint MipLevel;
} pc;

/* 
    Van Der Corput sequence
    Ref: https://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
 */
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

void main() {
    vec3 N = normalize(inPosition);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    float adaptFactor = max(pc.Roughness + pc.MipLevel / 4.0, 0.0);
    float maxBrightness = mix(20.0, 80.0, adaptFactor);

    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, pc.Roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0) {
            vec3 envSample = texture(g_environmentMap, L).rgb;
            float luminance = dot(envSample, vec3(0.2126, 0.7152, 0.0722));
            float maxColor = max(envSample.r, max(envSample.g, envSample.b));

            float brightness = max(luminance, maxColor * 0.5);
            if(brightness > maxBrightness) {
                envSample *= maxBrightness / brightness;
            }

            prefilteredColor += envSample * NdotL;

            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;
    outColor = vec4(prefilteredColor, 1.0);
}

