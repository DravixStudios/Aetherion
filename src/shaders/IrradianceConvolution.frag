#version 450
#define PI 3.14159265359

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform samplerCube g_environmentMap;

void main() {
    vec3 N = normalize(inWorldPos);
    vec3 irradiance = vec3(0.0);

    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    float sampleDelta = 0.02;
    int nrSamples = 0;

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = normalize(tangentSample.x * right + tangentSample.y * up + tangentSample.z * N);

            irradiance += texture(g_environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance / float(nrSamples);
    outColor = vec4(irradiance, 1.0);
}
