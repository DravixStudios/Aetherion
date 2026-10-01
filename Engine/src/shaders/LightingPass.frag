#version 450
#define PI 3.14159265359

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalImage;
layout(location = 1) out vec4 bloomThreshold;

/* Set 0: G-Buffers */
layout(set = 0, binding = 0) uniform sampler2D g_gbuffers[6];

/* Set 1: IBL Maps */
layout(set = 1, binding = 0) uniform samplerCube g_iblMaps[2]; // [0] Irradiance, [1] Prefilter
layout(set = 1, binding = 1) uniform sampler2D g_brdfLUT;

/* Set 2: Shadow maps */
layout(set = 2, binding = 0) uniform sampler2DArrayShadow g_shadowMap;
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

/*
    PCF (Percentage Closer Filtering)
    Samples a 3x3 kernel to smooth
    shadow borders
*/
float SampleShadowPCF(vec3 shadowCoord, int cascadeIdx) {
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(g_shadowMap, 0).xy);

    for(int x = -1; x <= 1; x++) {
        for(int y = -1; y <= 1; y++) {
            vec2 offset = vec2(x, y) * texelSize;

            shadow += texture(g_shadowMap, vec4(
                shadowCoord.xy + offset, // UV
                float(cascadeIdx), // Array layer
                shadowCoord.z // Depth for comparison
            ));
        }
    }

    return shadow / 9.0;
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
    int cascade = SelectCascade(viewDepth);

    float blendFactor = 0.1;

    float cascadeBias[4] = { 0.07, 0.085, 0.18, 0.2 };

    /* Apply normal offset bias to avoid shadow acne */
    float slopeBias = 0.02 * (1.0 - dot(normal, normalize(pc.sunDirection)));
    float bias = max(slopeBias, cascadeBias[cascade]);
    vec3 biasedPos = worldPos + normal * bias;

    /* Get the shadow of the cascade */
    float shadow = SampleShadowPCF(WorldToShadowUV(biasedPos, cascade), cascade);

    /* 
        Cascade blending 
        Calculate how much is left to
        arrive the end of the actual cascade
    */
    float nextSplit = cascades.cascadeSplits[cascade];
    float fadeRange = nextSplit * blendFactor;

    float fade = clamp((viewDepth - (nextSplit - fadeRange)) / fadeRange, 0.0, 1.0);

    if(fade > 0.0 && cascade < 3) {
        float nextShadow = SampleShadowPCF(WorldToShadowUV(biasedPos, cascade + 1), cascade + 1);
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

void main() {
    const vec3 albedo = texture(g_gbuffers[0], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;
    const vec3 N = normalize(texture(g_gbuffers[1], vec2(inUVs.x, 1.0 - inUVs.y)).rgb * 2.0 - 1.0);
    const vec3 orm = texture(g_gbuffers[2], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;
    const vec3 emissive = texture(g_gbuffers[3], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;
    const float depth = texture(g_gbuffers[4], vec2(inUVs.x, 1.0 - inUVs.y)).r;

    const vec4 clipPos = vec4(inUVs.x * 2.0 - 1.0, (1.0 - inUVs.y) * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = pc.invViewProjection * clipPos;
    viewPos /= viewPos.w;

    const vec3 position = viewPos.xyz;

    const vec4 bentNormalData = texture(g_gbuffers[5], vec2(inUVs.x, 1.0 - inUVs.y));
    const vec3 bentN = normalize(bentNormalData.xyz * 2.0 - 1.0);
    const float bentAO = bentNormalData.a;

    const float ao = orm.r;
    const float roughness = clamp(orm.g, 0.05, 1.0);
    const float metalness = orm.b;

    const vec3 correctedCameraPos = vec3(pc.cameraPosition.x, pc.cameraPosition.y, -pc.cameraPosition.z);
    
    const vec3 V = normalize(correctedCameraPos - position);
    const vec3 R = reflect(-V, N);

    const uint debugViewType = (pc.debugView >> 16) & 0xFFFFu;
    const uint debugView = (pc.debugView & 0xFFFFu);

    vec3 directDiffuse = vec3(0.f);
    vec3 indirectDiffuse = vec3(0.f);

    vec3 directSpecular = vec3(0.f);
    vec3 indirectSpecular = vec3(0.f);

    const uint debugViewType = (pc.debugView >> 16) & 0xFFFFu;
    const uint debugView = (pc.debugView & 0xFFFFu);

    if (debugViewType == DEBUG_VIEW_TYPE_GBUFFER && debugView != DEFAULT_VIEW) {
        vec3 debugColor = vec3(0.f);
        switch (debugView) {
            case DEBUG_VIEW_ALBEDO:
                debugColor = albedo;
                break;
            case DEBUG_VIEW_NORMAL:
                debugColor = N;
                break;
            case DEBUG_VIEW_BENT_NORMAL:
                debugColor = bentN;
                break;
            case DEBUG_VIEW_BENT_NORMAL_AO:
                debugColor = vec3(bentAO, bentAO, bentAO);
                break;
            case DEBUG_VIEW_EMISSIVE:
                debugColor = emissive;
                break;
            case DEBUG_VIEW_DEPTH:
                debugColor = vec3(depth, depth, depth);
                break;
            default:
                debugColor = vec3(0.0);
                break;
        }

        finalImage = vec4(debugColor, 1.f);
        return;
    }

    vec3 directDiffuse = vec3(0.f);
    vec3 indirectDiffuse = vec3(0.f);

    vec3 directSpecular = vec3(0.f);
    vec3 indirectSpecular = vec3(0.f);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

    /* ==== DIRECTIONAL LIGHT ==== */
    vec3 color = vec3(0.0);
    vec3 Lo = vec3(0.0);

    /* SUN LIGHT */
    {
        const vec3 L = normalize(pc.sunDirection);
        const vec3 H = normalize(V + L);

        const float HdotV = clamp(dot(H, V), 0.0, 1.0);
        const float NdotL = max(dot(N, L), 0.0);
        const float NdotV = max(dot(N, V), 0.0);

        const float NDF = DistributionGGX(N, H, roughness);
        const float G = GeometrySmith(N, V, L, roughness);
        const vec3 F = FresnelSchlick(HdotV, F0);
        
        const vec3 numerator = NDF * G * F;
        const float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;

        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metalness);

        const vec3 specular = numerator / denominator;
        const vec3 diffuse = albedo * DiffuseDisney(NdotL, NdotV, roughness);

        /* Calculate view-space depth for selecting the cascade */
        const float viewDepth = length(correctedCameraPos - position);
        const float shadow = CalculateShadow(position, N, viewDepth);

        /* Sun radiance (color * intensity * shadow) */
        const vec3 sunRadiance = vec3(1.0) * pc.sunIntensity;
        Lo += (kD * diffuse + specular) * sunRadiance * NdotL * shadow;
        directSpecular += (specular * sunRadiance * NdotL * shadow);
        directDiffuse += (kD * diffuse * sunRadiance * NdotL * shadow);
    }

    color += Lo;

    /* ==== IBL (Ambient lighting for environment) ==== */
    vec3 ambient = vec3(0.0);
    {
        const float MAX_REFLECTION_LOD = 4.0;
        const vec3 irradiance = texture(g_iblMaps[0], vec3(-bentN.x, bentN.y, bentN.z)).rgb;
        const vec3 prefilteredColor = textureLod(g_iblMaps[1], vec3(R.x, R.y, R.z), roughness * MAX_REFLECTION_LOD).rgb;
        const vec2 brdf = texture(g_brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;

        const vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metalness);

        /* Diffuse IBL */
        const vec3 diffuse = irradiance * albedo;
    
        /* Specular IBL */
        const vec3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);
        ambient += (kD * diffuse + specular) * bentAO;
        indirectSpecular += specular * bentAO;
        indirectDiffuse += (kD * diffuse * bentAO);
    }


    /* ==== COMBINE ==== */
    color += ambient;

    color += emissive;

    if (debugViewType == DEBUG_VIEW_TYPE_LIGHTING && debugView != DEFAULT_VIEW) {
        vec3 debugColor = vec3(0.f);
        switch (debugView) {
            case DEBUG_VIEW_DIRECT_SPECULAR:
                debugColor = directSpecular;
                break;

            case DEBUG_VIEW_INDIRECT_SPECULAR:
                debugColor = indirectSpecular;
                break;

            case DEBUG_VIEW_DIRECT_DIFFUSE:
                debugColor = directDiffuse;
                break;

            case DEBUG_VIEW_INDIRECT_DIFFUSE:
                debugColor = indirectDiffuse;
                break;

            default:
                debugColor = vec3(0.0);
                break;
        }

        finalImage = vec4(vec3(debugColor), 1.0);
        return;
    }

    finalImage = vec4(vec3(color), 1.0);
    // finalImage = vec4(bentAO, bentAO, bentAO, 1.0);
    finalImage = vec4(vec3(color), 1.0);

    const float distanceToCamera = length(correctedCameraPos - position);
    const float kBloomFadeStart = 5.f; // TODO: Promote to a variable
    const float kBloomFadeEnd = 10.f; // TODO: Promote to a variable

    const float bloomDistanceFactor = 1.f - smoothstep(kBloomFadeStart, kBloomFadeEnd, distanceToCamera);

    const vec3 luma = vec3(0.2126f, 0.7152f, 0.0722f);
    const float kLuminanceThreshold = 1.f;

    const float luminance = dot(color, luma);
    if (luminance > kLuminanceThreshold) {
        bloomThreshold = vec4(color * bloomDistanceFactor, 1.f);
    }
}
