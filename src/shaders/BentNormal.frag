#version 450
#define PI 3.14159265359

layout(location = 0) in vec2 inUVs;
layout(location = 0) out vec4 outBentNormals;

layout(set = 0, binding = 0) uniform sampler2D g_normal;
layout(set = 0, binding = 1) uniform sampler2D g_depth;

layout(set = 1, binding = 0) uniform CameraData {
    mat4 invView;
    mat4 invProj;
    mat4 proj;
    float radius;
} camData;

vec3 GetViewPosition(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = camData.invProj * clipPos;
    return viewPos.xyz / viewPos.w;
}

/* 
    From: Hash wothout sine
    Ref: https://compute.toys/view/15
*/
float Hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 RotateDirection(vec2 dir, vec2 cosSin) {
    return vec2(
        dir.x * cosSin.x - dir.y * cosSin.y,
        dir.x * cosSin.y + dir.y * cosSin.x
    );
}

float ComputeHorizon(vec3 pos, vec3 normal, vec3 dir, float maxDist, out float outDist) {
    const int STEPS = 4;

    float maxHorizonCos = -1.0;
    outDist = maxDist;

    for(int step = 1; step <= STEPS; ++step) {
        float stepSize = (float(step) / float(STEPS)) * maxDist;

        vec3 samplePos = pos + dir * stepSize;

        /* Project to screen */
        vec4 clipPos = camData.proj * vec4(samplePos, 1.0);
        vec2 sampleUV = (clipPos.xy / clipPos.w) * 0.5 + 0.5;

        if(sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            continue;
        }

        float sampleDepth = texture(g_depth, sampleUV).r;
        vec3 occluderPos = GetViewPosition(vec2(sampleUV.x, sampleUV.y), sampleDepth);

        vec3 horizonDir = occluderPos - pos;
        float horizonDist = length(horizonDir);
        horizonDir = normalize(horizonDir);

        float horizonCos = dot(horizonDir, normal);

        if(horizonCos > maxHorizonCos) {
            maxHorizonCos = horizonCos;
            outDist = horizonDist;
        }
    }

    return maxHorizonCos;
}

void main() {
    const int SLICE_COUNT = 4;

    vec2 sampleUV = vec2(inUVs.x, 1.0 - inUVs.y);
    vec3 N = normalize(texture(g_normal, sampleUV).xyz * 2.0 - 1.0);
    float depth = texture(g_depth, sampleUV).r;

    if(depth >= 1.0) {
        outBentNormals = vec4(0.5, 0.5, 1.0, 0.0);
        return;
    }

    vec3 viewPos = GetViewPosition(sampleUV, depth);
    vec3 normalView = normalize(mat3(transpose(camData.invView)) * N);

    /* Random per pixel rotation */
    float randomAngle =  Hash(gl_FragCoord.xy) * 2.0 * PI;
    vec2 randomDir = vec2(cos(randomAngle), sin(randomAngle));

    vec3 bentNormalAccum = vec3(0.0);
    float totalOcclusion = 0.0;

    /* TBN for slice directions */ 
    vec3 basis = (abs(normalView.x) < 0.5) ? vec3(1, 0, 0) : (abs(normalView.y) < 0.5) ? vec3(0, 1, 0) : vec3(0, 0, 1);
    vec3 tangent = normalize(cross(basis, normalView));
    vec3 bitangent = cross(normalView, tangent);

    for(int slice = 0; slice < SLICE_COUNT; ++slice) {
        float sliceAngle = (float(slice) / float(SLICE_COUNT)) * PI;

        /* Slice direction in view space */
        vec2 sliceDir2D = vec2(cos(sliceAngle), sin(sliceAngle));
        sliceDir2D = RotateDirection(sliceDir2D, vec2(randomDir.x, randomDir.y));

        vec3 sliceDir = tangent * sliceDir2D.x + bitangent * sliceDir2D.y;

        /* Calculate horizon in both directions (+/-) */
        float dist1, dist2;
        float horizonCos1 = ComputeHorizon(viewPos, normalView, sliceDir, camData.radius, dist1);
        float horizonCos2 = ComputeHorizon(viewPos, normalView, -sliceDir, camData.radius, dist2);

        horizonCos1 = max(horizonCos1, 0.0);
        horizonCos2 = max(horizonCos2, 0.0);

        /* Occlusion */ 
        float angle1 = asin(clamp(horizonCos1, -1.0, 1.0));
        float angle2 = asin(clamp(horizonCos2, -1.0, 1.0));

        float sliceOcclusion = (angle1 + angle2) / PI;

        float falloff1 = 1.0 - smoothstep(0.0, camData.radius, dist1);
        float falloff2 = 1.0 - smoothstep(0.0, camData.radius, dist2);
        float avgFalloff = (falloff1 + falloff2) * 0.5;

        sliceOcclusion *= avgFalloff;

        float sliceVisibility = 1.0 - sliceOcclusion;
        totalOcclusion += sliceOcclusion;

        /* Bent normal contribution */
        float bentAngle = (angle1 - angle2) * 0.5;
        vec3 bentDir = sliceDir * sin(bentAngle);

        bentNormalAccum += bentDir * sliceVisibility;
    }

    float occlusion = totalOcclusion / float(SLICE_COUNT);
    float finalAO = 1.0 - occlusion;

    vec3 finalBentNormal;
    if(length(bentNormalAccum) > 0.001) {
        finalBentNormal = normalize(normalView + bentNormalAccum * 0.15);
    } else {
        finalBentNormal = normalView;
    }

    vec3 bentNormalWorld = normalize(mat3(camData.invView) * finalBentNormal);

    finalAO = pow(finalAO, 1.2);
    
    outBentNormals = vec4(
        bentNormalWorld * 0.5 + 0.5,
        finalAO
    );
}