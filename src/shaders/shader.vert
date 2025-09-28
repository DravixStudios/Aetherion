#version 450

/* Input */
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec2 inUVs;

/* Output */
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec2 outUVs;

void main() {
    gl_Position = vec4(inPosition,  1.0);
    fragColor = vec4(1.0, 0.0, 0.0, 0.0);

    outNormals = inNormals;
    outUVs = inUVs;
}