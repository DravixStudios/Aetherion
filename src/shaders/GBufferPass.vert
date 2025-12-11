#version 450

/* Input locations */
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormals;
layout(location = 2) in vec2 inUVs;

/* Get our WVP from our binding 0 */ 
layout(binding = 0) uniform WVP {
    mat4 World;
    mat4 View;
    mat4 Projection;
} wvp;

/* Output locations */
layout(location = 0) out vec3 outNormals;
layout(location = 1) out vec2 outUVs;
layout(location = 2) out vec3 fragPos;

void main() {
    /* Multiply our vertex position by our wold matrix */ 
    vec4 worldPos = wvp.World * vec4(inPosition, 1.0);

    /* Multiply our Porjection * View * Vertex World position */ 
    gl_Position = wvp.Projection * wvp.View * worldPos;

    /* Fragment position = Vertex world position */ 
    fragPos = worldPos.xyz;

    /* Invert our world matrix and transpose it */ 
    mat3 normalMatrix = transpose(inverse(mat3(wvp.World)));
    outNormals = normalize(normalMatrix * inNormals); // Normal matrix * input normals. Then normalize normals
    
    outUVs = inUVs;
}