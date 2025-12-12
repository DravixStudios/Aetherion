#version 450

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalImage;

layout(set = 0, binding = 0) uniform sampler2D g_gbuffers[3];

const vec3 lightPos = vec3(1.0, 1.0, 1.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float ambientStrength = 0.1;

void main() {
    vec3 albedo = texture(g_gbuffers[0], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;
    vec3 normal = normalize(texture(g_gbuffers[1], vec2(inUVs.x, 1.0 - inUVs.y)) * 2.0 - 1.0).rgb;
    vec3 position = texture(g_gbuffers[2], vec2(inUVs.x, 1.0 - inUVs.y)).rgb;

    vec3 lightDir = normalize(lightPos - position);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo * lightColor;

    vec3 ambient = ambientStrength * albedo;
    vec3 color = ambient + diffuse;


    finalImage = vec4(color, 1.0);
}