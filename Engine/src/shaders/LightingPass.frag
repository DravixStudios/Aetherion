#version 450
const float PI = 3.14159265359;
const float TWO_PI = PI * 2;

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalImage;
layout(location = 1) out vec4 bloomThreshold;

/* Set 0: G-Buffers */
layout(set = 0, binding = 0) uniform sampler2DMS g_gbuffers[5];
layout(set = 0, binding = 5) uniform sampler2DMS g_depth;

/* Set 1: IBL Maps */
layout(set = 1, binding = 0) uniform samplerCube g_iblMaps[2]; // [0] Irradiance, [1] Prefilter
layout(set = 1, binding = 1) uniform sampler2D g_brdfLUT;

/* Set 2: Shadow maps */
layout(set = 2, binding = 0) uniform sampler2DArray g_shadowMap;
layout(set = 2, binding = 1) uniform CascadeData {
    mat4 cascadeViewProj[4];
    vec4 cascadeSplits; // x=split0, y=split1, z=split2, w=split3
} cascades;

layout(push_constant) uniform PushConstants {
    mat4 invViewProjection;
    vec4 cameraPosition;
    vec3 sunDirection;
    float sunIntensity;
    uint debugView;
    uint sampleCount;
} pc;


const uint DEFAULT_VIEW = 0xFFFF;

// Debug view types
const uint DEBUG_VIEW_TYPE_GBUFFER = 1;
const uint DEBUG_VIEW_TYPE_LIGHTING = 1 << 1;

// G-Buffer debug views
const uint DEBUG_VIEW_ALBEDO = 1;
const uint DEBUG_VIEW_NORMAL = 1 << 1;
const uint DEBUG_VIEW_ORM = 1 << 2;
const uint DEBUG_VIEW_EMISSIVE = 1 << 3;
const uint DEBUG_VIEW_BENT_NORMAL = 1 << 4;
const uint DEBUG_VIEW_BENT_NORMAL_AO = 1 << 5;
const uint DEBUG_VIEW_DEPTH = 1 << 6;

// Lighting debug views
const uint DEBUG_VIEW_DIRECT_SPECULAR = 1;
const uint DEBUG_VIEW_INDIRECT_SPECULAR = 1 << 1;
const uint DEBUG_VIEW_DIRECT_DIFFUSE = 1 << 2;
const uint DEBUG_VIEW_INDIRECT_DIFFUSE = 1 << 3;

// Light info 
// TODO: Promote this to a light component and per-light calculations
const vec3 lightPos = vec3(0.0, 3.0, 3.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

/*
    Interleaved Gradient Noise

    Ref: Next generation Post processing in Call Of Duty: Advanced Warfare - Jorge Jimenez
*/
float InterleavedGradientNoise(vec2 pos) {
    return fract(52.9829189 * fract(0.06711056 * pos.x + 0.00583715 * pos.y));
}

float SampleShadowVogel(vec3 shadowCoord, int cascadeIdx) {
    const vec2 texelSize = 1.0 / vec2(textureSize(g_shadowMap, 0).xy);
    const float noise = InterleavedGradientNoise(gl_FragCoord.xy) * TWO_PI;
    const mat2 rot = mat2(cos(noise), -sin(noise), sin(noise), cos(noise));

    float shadow = 0.0;
    const int SAMPLES = 8;
    const float GOLDEN_ANGLE = 2.399632;
    const float VOGEL_RADIUS = 2.0;

    for(int i = 0; i < SAMPLES; i++) {
        float r = sqrt(float(i) + 0.5) / sqrt(float(SAMPLES));
        float theta = float(i) * GOLDEN_ANGLE;
        vec2 offset = rot * (vec2(cos(theta), sin(theta)) * r) * texelSize * VOGEL_RADIUS;

        float depth = texture(g_shadowMap, vec3(shadowCoord.xy + offset, float(cascadeIdx))).r;
        shadow += (shadowCoord.z <= depth) ? 1.0 : 0.0;
    }

    return shadow / float(SAMPLES);
}

/*
    Select the correct cascade depending
    on the fragment distance to the 
    camera
*/
int SelectCascade(float viewDepth) {
    for(int i = 0; i < 4; i++) {
        if(viewDepth < cascades.cascadeSplits[i]) {
            return i;
        }
    }
    
    return 3;
}

vec3 WorldToShadowUV(vec3 worldPos, int cascade) {
    vec4 shadowPos = cascades.cascadeViewProj[cascade] * vec4(worldPos, 1.0);
    vec3 projCoords = shadowPos.xyz / shadowPos.w;

    return vec3(projCoords.xy * 0.5 + 0.5, projCoords.z);
}

/*
    Calculates the shadow factor for
    a given fragment

    Returns 1.0 = illuminated, 0.0 = shaded
*/
float CalculateShadow(vec3 worldPos, vec3 normal, float viewDepth) {
    int cascade = SelectCascade(abs(viewDepth));

    float blendFactor = 0.1;

    float cascadeBias[4] = { 0.07, 0.085, 0.18, 0.2 };

    /* Apply normal offset bias to avoid shadow acne */
    float slopeBias = 0.02 * (1.0 - dot(normal, normalize(pc.sunDirection)));
    float bias = max(slopeBias, cascadeBias[cascade]);
    vec3 biasedPos = worldPos + normal * bias;

    /* Get the shadow of the cascade */
    float shadow = SampleShadowVogel(WorldToShadowUV(biasedPos, cascade), cascade);

    /* 
        Cascade blending 
        Calculate how much is left to
        arrive the end of the actual cascade
    */
    float nextSplit = cascades.cascadeSplits[cascade];
    float fadeRange = nextSplit * blendFactor;

    float fade = clamp((viewDepth - (nextSplit - fadeRange)) / fadeRange, 0.0, 1.0);

    if(fade > 0.0 && cascade < 3) {
        float nextShadow = SampleShadowVogel(WorldToShadowUV(biasedPos, cascade + 1), cascade + 1);
        shadow = mix(shadow, nextShadow, fade);
    }

    return shadow;
}

/* 
    Simplified Disney Diffuse (Burley) 

    https://disneyanimation.com/publications/physically-based-shading-at-disney/
*/
float DiffuseDisney(float NdotL, float NdotV, float roughness) {
    float FL = pow(1.0 - NdotL, 5.0);
    float FV = pow(1.0 - NdotV, 5.0);

    float FD90 = 0.5 + 2 * roughness;

    float fd = mix(1.0, FD90, FL) * mix(1.0, FD90, FV);

    return fd / PI;
}

vec3 CalculateSamplePBR(ivec2 p, vec2 sampledUV, int sampleIdx,
        out vec3 outDirectSpec, out vec3 outIndirectSpec, out vec3 outDirectDiff, out vec3 outIndirectDiff) {
    outDirectSpec = vec3(0.f);
    outIndirectSpec = vec3(0.f);
    outDirectDiff = vec3(0.f);
    outIndirectDiff = vec3(0.f);

    const float subpixelDepth = texelFetch(g_depth, p, sampleIdx).r;
    if(subpixelDepth >= 1.f) return vec3(0.f);

    const vec3 albedo = texelFetch(g_gbuffers[0], p, sampleIdx).rgb;
    const vec3 N = normalize(texelFetch(g_gbuffers[1], p, sampleIdx).rgb * 2.0 - 1.0);
    const vec3 orm = texelFetch(g_gbuffers[2], p, sampleIdx).rgb;
    const vec3 emissive = texelFetch(g_gbuffers[3], p, sampleIdx).rgb;

    const vec4 clipPos = vec4(inUVs.x * 2.0 - 1.0, (1.0 - inUVs.y) * 2.0 - 1.0, subpixelDepth, 1.0);
    vec4 viewPos = pc.invViewProjection * clipPos;
    viewPos /= viewPos.w;
    const vec3 position = viewPos.xyz;

    const vec4 bentNormalData = texelFetch(g_gbuffers[4], p, sampleIdx);
    const vec3 bentN = normalize(bentNormalData.xyz * 2.0 - 1.0);
    const float bentAO = bentNormalData.a;

    const float ao = orm.r;
    const float roughness = clamp(orm.g, 0.05, 1.0);
    const float metalness = orm.b;

    const vec3 correctedCameraPos = vec3(pc.cameraPosition.x, pc.cameraPosition.y, -pc.cameraPosition.z);

    vec3 Lo =  vec3(0.f);
    vec3 color = vec3(0.f);
    vec3 ambient = vec3(0.f);

    const vec3 V = normalize(correctedCameraPos - position);
    const vec3 R = reflect(-V, N);

    const vec3 L = normalize(pc.sunDirection);
    const vec3 H = normalize(V + L);

    const float HdotV = clamp(dot(H, V), 0.0, 1.0);
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotV = max(dot(N, V), 0.0);
    const float LdotH = max(dot(L, H), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

    {
        const float viewDepth = length(correctedCameraPos - position);
        const float shadow = CalculateShadow(position, N, viewDepth);

        const float NDF = DistributionGGX(N, H, roughness);
        const float G = GeometrySmith(N, V, L, roughness);
        const vec3 F = FresnelSchlick(HdotV, F0);

        const vec3 numerator = NDF * G * F;
        const float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;

        const vec3 specular = numerator / denominator;
        const vec3 diffuse = albedo * DiffuseDisney(NdotL, NdotV, roughness);

        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metalness);

        const vec3 sunRadiance = vec3(1.0) * pc.sunIntensity;

        Lo += (kD * diffuse + specular) * sunRadiance * NdotL * shadow;

        outDirectDiff += (kD * diffuse * sunRadiance * NdotL * shadow);
        outDirectSpec += (specular * sunRadiance * NdotL * shadow);
    }

    {
        const float MAX_REFLECTION_LOD = 4.0;
        const vec3 irradiance = texture(g_iblMaps[0], vec3(-bentN.x, bentN.y, bentN.z)).rgb;
        const vec3 prefilteredColor = textureLod(g_iblMaps[1], vec3(R.x, R.y, R.z), roughness * MAX_REFLECTION_LOD).rgb;
        const vec2 brdf = texture(g_brdfLUT, vec2(NdotV, roughness)).rg;

        const vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metalness);

        /* Diffuse IBL */
        const vec3 diffuse = irradiance * albedo;

        /* Specular IBL */
        const vec3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);
        ambient += (kD * diffuse + specular) * bentAO;

        outIndirectDiff += kD * irradiance * albedo * ao * bentAO;
        outIndirectSpec += specular * bentAO;
    }

    return Lo + ambient + emissive;
}

void main() {
    const ivec2 p = ivec2(gl_FragCoord.xy);
    const vec2 sampledUV = vec2(inUVs.x, 1.f - inUVs.y);

    const uint debugViewType = (pc.debugView >> 16) & 0xFFFFu;
    const uint debugView = (pc.debugView & 0xFFFFu);

    if(debugViewType == DEBUG_VIEW_TYPE_GBUFFER && debugView != DEFAULT_VIEW) {
        vec3 debugColor = vec3(0.f);
        switch (debugView) {
            case DEBUG_VIEW_ALBEDO:        debugColor = texelFetch(g_gbuffers[0], p, 0).rgb; break;
            case DEBUG_VIEW_NORMAL:        debugColor = normalize(texelFetch(g_gbuffers[1], p, 0).rgb * 2.0 - 1.0); break;
            case DEBUG_VIEW_ORM:           debugColor = texelFetch(g_gbuffers[2], p, 0).rgb; break;
            case DEBUG_VIEW_EMISSIVE:      debugColor = texelFetch(g_gbuffers[3], p, 0).rgb; break;
            case DEBUG_VIEW_BENT_NORMAL:   debugColor = normalize(texelFetch(g_gbuffers[4], p, 0).xyz * 2.0 - 1.0); break;
            case DEBUG_VIEW_BENT_NORMAL_AO:debugColor = vec3(texelFetch(g_gbuffers[4], p, 0).a); break;
            case DEBUG_VIEW_DEPTH:         debugColor = vec3(texelFetch(g_depth, p, 0).r); break;
        }

        finalImage = vec4(debugColor, 1.f);
        return;
    }

    const float depth0 = texelFetch(g_depth, p, 0).r;
    const float depth1 = texelFetch(g_depth, p, int(pc.sampleCount - 1)).r;

    const vec3 normal0 = normalize(texelFetch(g_gbuffers[1], p, 0).xyz * 2.0 - 1.0);
    const vec3 normal1 = normalize(texelFetch(g_gbuffers[1], p, int(pc.sampleCount - 1)).xyz * 2.0 - 1.0);

    const bool isEdge = (abs(depth0 - depth1) > .005f) || (dot(normal0, normal1) < .95f);

    vec3 colorAccum = vec3(0.f);
    vec3 dSpec, iSpec, dDiff, iDiff;

    // TODO: Calculate bloom
    const vec3 baseSampleColor = CalculateSamplePBR(p, sampledUV, 0, dSpec, iSpec, dDiff, iDiff);
    vec3 bloom = vec3(0.f);
    bloomThreshold += vec4(bloom, 1.f);

    if(isEdge) {
        colorAccum = baseSampleColor;

        for(uint s = 1; s < pc.sampleCount; ++s) {
            colorAccum += CalculateSamplePBR(p, sampledUV, int(s), dSpec, iSpec, dDiff, iDiff);
        }

        colorAccum /= float(pc.sampleCount);
    } else {
        colorAccum += baseSampleColor;
        if (debugViewType == DEBUG_VIEW_TYPE_LIGHTING && debugView != DEFAULT_VIEW) {
            switch (debugView) {
                case DEBUG_VIEW_DIRECT_SPECULAR:   colorAccum = dSpec; break;
                case DEBUG_VIEW_INDIRECT_SPECULAR: colorAccum = iSpec; break;
                case DEBUG_VIEW_DIRECT_DIFFUSE:    colorAccum = dDiff; break;
                case DEBUG_VIEW_INDIRECT_DIFFUSE:  colorAccum = iDiff; break;
            }
            finalImage = vec4(colorAccum, 1.f);
            return;
        }
    }

//    const float distanceToCamera = length(correctedCameraPos - position);
//    const float kBloomFadeStart = 5.f; // TODO: Promote to a variable
//    const float kBloomFadeEnd = 10.f; // TODO: Promote to a variable
//
//    const float bloomDistanceFactor = 1.f - smoothstep(kBloomFadeStart, kBloomFadeEnd, distanceToCamera);
//
//    const vec3 luma = vec3(0.2126f, 0.7152f, 0.0722f);
//    const float kLuminanceThreshold = 1.f;

//    const float luminance = dot(color, luma);
//    if (luminance > kLuminanceThreshold) {
//        bloomThreshold = vec4(color * bloomDistanceFactor, 1.f);
//    }

    finalImage = vec4(colorAccum, 1.f);
}
