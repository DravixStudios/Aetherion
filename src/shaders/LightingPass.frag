#version 450
#define PI 3.14159265359

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalImage;

layout(set = 0, binding = 0) uniform sampler2D g_gbuffers[5];
layout(set = 0, binding = 1) uniform samplerCube g_irradianceMap;
layout(set = 0, binding = 2) uniform samplerCube g_prefilterMap;
layout(set = 0, binding = 3) uniform sampler2D g_brdfLUT;

layout(push_constant) uniform PushConstants {
    vec3 cameraPosition;
} pc;

const vec3 lightPos = vec3(0.0, 3.0, 3.0);
const vec3 lightColor = vec3(20.0, 20.0, 20.0);

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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

void main() {
    vec3 albedo = texture(g_gbuffers[0], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;
    vec3 N = normalize(texture(g_gbuffers[1], vec2(inUVs.x, 1.0 - inUVs.y)).rgb * 2.0 - 1.0);
    vec3 orm = texture(g_gbuffers[2], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;
    vec3 emissive = texture(g_gbuffers[3], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;
    vec3 position = texture(g_gbuffers[4], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;

    vec3 color = vec3(0.0);

    float ao = orm.r;
    float roughness = clamp(orm.g, 0.05, 1.0);
    float metalness = orm.b;

    vec3 V = normalize(pc.cameraPosition - position);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

    vec3 L = normalize(lightPos - position);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - position);
    float attenuation = 1.0 / (distance * distance + 0.1);
    vec3 radiance = lightColor * attenuation;
    radiance *= (1.0 + metalness * 0.2);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metalness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    color += Lo;

    vec3 ambient = vec3(0.05) * albedo * ao;
    color += ambient;

    color += emissive;

    color = color / (color + 1.0);
    // color = pow(color, vec3(1.0/2.2));

    finalImage = vec4(color, 1.0);
}