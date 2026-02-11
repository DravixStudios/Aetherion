#version 450

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 finalColor;

layout(set = 0, binding = 0) uniform sampler2D g_hdrColor;

void main() {
    vec3 hdrColor = texture(g_hdrColor, inUVs).rgb;

    /* Tonemapping */
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

    /* Gamma correction */
    mapped = pow(mapped, vec3(1.0 / 2.2));

    finalColor = vec4(mapped, 1.0);
}