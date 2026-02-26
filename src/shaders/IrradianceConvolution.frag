#version 450
#define PI 3.14159265359
#define MAX_BRIGHTNESS 50.0

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform samplerCube g_environmentMap;

void main() {
    vec3 N = normalize(inWorldPos);
    vec3 irradiance = vec3(0.0);

    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    const float TWO_PI = PI * 2.0;
    const float HALF_PI = PI * 0.5;

    float sampleDelta = 0.1;
    float nrSamples = 0.0;

    for(float phi = 0.0; phi < TWO_PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < HALF_PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );
            
            vec3 sampleVec = tangentSample.x * right + 
                           tangentSample.y * up + 
                           tangentSample.z * N;

            vec3 envColor = texture(g_environmentMap, sampleVec).rgb;
            
            float brightness = max(envColor.r, max(envColor.g, envColor.b));

            if(brightness > MAX_BRIGHTNESS) {
                envColor *= MAX_BRIGHTNESS / brightness;
            }
            
            irradiance += envColor * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance / nrSamples;
    
    outColor = vec4(irradiance, 1.0);
}