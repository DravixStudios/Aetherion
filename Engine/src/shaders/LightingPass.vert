#version 450

/* Input locations */
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUVs;

/* Output locations */
layout(location = 0) out vec2 outUVs;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    outUVs = inUVs;
}