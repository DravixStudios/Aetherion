#version 450
#define PI 3.14159265359

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalColor;

layout(set = 0, binding = 0) uniform sampler2D g_hdrColor;
layout(set = 0, binding = 1) uniform sampler2D g_emissiveColor;

// TODO: Promote to variables
const float sigma = 1.5;
const float blurRadius = 1.0;

float Gaussian1D(float x, float sig) {
    float sig2 = sig * sig;
    return (1.0 / (sig * sqrt(2.0 * PI))) * exp(-(x * x) / (2.0 * sig2));
}

void main() {
    vec2 uv = inUVs;
    vec3 hdrColor = texture(g_hdrColor, uv).rgb;
    vec3 nativeEmissive = texture(g_emissiveColor, uv).rgb;

    vec2 texelSize = 1.f / vec2(textureSize(g_emissiveColor, 0));
    vec3 blurredBloom = vec3(0.f);

    float totalWeight = 0.f;
    for(int x = -2; x <= 2; x++) {
        for(int y = -2; y <= 2; y++) {
            float weightX = Gaussian1D(float(x), sigma);
            float weightY = Gaussian1D(float(y), sigma);

            float weight = weightX * weightY;

            vec2 offset = vec2(float(x), float(y)) * texelSize * blurRadius;

            blurredBloom += texture(g_emissiveColor, uv + offset).rgb * weight;
            totalWeight += weight;
        }
    }
    blurredBloom /= totalWeight;

    float intensity = clamp(length(nativeEmissive), 0.0, 1.0);
    float dynamicMultiplier = mix(5.f, .5f, intensity); // TODO: Promote to variable

    vec3 mixedColor = hdrColor + (blurredBloom * dynamicMultiplier); // TODO: Promote to variable

    /* Tonemapping */
    vec3 mapped = mixedColor / (mixedColor + vec3(1.0));

    /* Gamma correction */
    mapped = pow(mapped, vec3(1.0 / 2.2));

    finalColor = vec4(mapped, 1.0);
}