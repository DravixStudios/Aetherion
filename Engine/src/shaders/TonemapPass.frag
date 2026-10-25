#version 450
#define PI 3.14159265359

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalColor;

layout(set = 0, binding = 0) uniform sampler2DMS g_hdrColor;
layout(set = 0, binding = 1) uniform sampler2D g_emissiveColor;

layout(push_constant) uniform PushConstants {
    uint sampleCount;
} pc;

// TODO: Promote to variables
const float sigma = 1.5;
const float blurRadius = 1.0;

float Gaussian1D(float x, float sig) {
    float sig2 = sig * sig;
    return (1.0 / (sig * sqrt(2.0 * PI))) * exp(-(x * x) / (2.0 * sig2));
}

/*
    ACES Approximation (Krzysztof Narkowicz)
    Ref: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
*/
// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESInputMat = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3(
        1.60475, -0.10208, -0.00327,
        -0.53108, 1.10813, -0.07276,
        -0.07367, -0.00605, 1.07602);

vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFitted(vec3 color) {
    color = ACESInputMat * color;
    color = RRTAndODTFit(color);
    color = ACESOutputMat * color;

    return clamp(color, 0.0, 1.0);
}

void main() {
    ivec2 texSize = textureSize(g_hdrColor);
    const ivec2 p = ivec2(gl_FragCoord.x, texSize.y - 1 - int(gl_FragCoord.y));
    vec2 uv = vec2(inUVs.x, 1.f - inUVs.y);
    vec3 nativeEmissive = texture(g_emissiveColor, uv).rgb;

    vec2 texelSize = 1.f / vec2(textureSize(g_emissiveColor, 0));
    vec3 blurredBloom = vec3(0.f);

    vec3 hdrColor = vec3(0.f);
    for(uint s = 0; s < pc.sampleCount; ++s) {
        vec4 subpixelColor = texelFetch(g_hdrColor, p, int(s));
        hdrColor += subpixelColor.xyz;
    }
    hdrColor /= pc.sampleCount;

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

    vec3 mixedColor = hdrColor; // TODO: Promote to variable

    /* Tonemapping */
    vec3 mapped = ACESFitted(mixedColor);

    /* Gamma correction */
    mapped = pow(mapped, vec3(1.0 / 2.2));

    finalColor = vec4(mapped, 1.0);
}