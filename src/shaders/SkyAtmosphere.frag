#version 450

layout(location = 0) in vec3 inDir;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform SunData {
    vec3 sunDir;
    float sunHeight;
} sunData;

const float PI = 3.14159265359;
const float R_EARTH = 6371e3; // Earth radius
const float R_ATMOS = 6471e3; // 100km of atmosphere
const float H_RAY = 8000.0; // Rayleigh height
const float H_MIE = 1200.0; // MIE height
const vec3 BETA_RAY = vec3(5.8e-6, 13.5e-6, 33.1e-6); // Rayleigh dispersion per RGB channel
const float BETA_MIE = 4e-6; // Mie dispersion
const float MIE_G = 0.76; // Henyey-Greenstein anisotropy

const int VIEW_STEPS = 32;
const int SUN_STEPS = 12;

/* 
    From: Hash without sine
    Ref: https://compute.toys/view/15
*/
float Hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

/**
* @param ro Ray origin
* @param rd Ray direction
* @param r Radius
*/
vec2 RaySphere(vec3 ro, vec3 rd, float r) {
    float b = dot(ro, rd);
    float c = dot(ro, ro) - r * r;
    float d = b * b - c;

    if(d < 0.0) return vec2(-1.0);
    
    d = sqrt(d);

    return vec2(-b - d, -b + d);
}

vec2 LocalDensity(vec3 p) {
    float h = max(length(p) - R_EARTH, 0.0);
    return vec2(exp(-h / H_RAY), exp(-h / H_MIE));
}

float PhaseRay(float c) {
    return (3.0 / (16.0 * PI)) * (1.0 + c * c);
}

float PhaseMie(float c) {
    float g = MIE_G;
    float g2 = g * g;

    float c2 = c * c;

    float oneMinusG2 = 1.0 - g2;
    float onePlusC2 = 1.0 + c2;

    float denomBase = 1.0 + g2 - 2.0 * g * c;
    float denom = pow(denomBase, 1.5);

    float norm = 3.0 / (8.0 * PI);
    float mieTerm = oneMinusG2 / (2.0 + g2);

    return norm * mieTerm * onePlusC2 / denom;
}

vec3 SunTransmittance(vec3 p) {
    vec2 hit = RaySphere(p, sunData.sunDir, R_ATMOS);
    if(hit.y < 0.0) return vec3(0.0);

    float seg = hit.y / float(SUN_STEPS);
    float t = 0.0;
    vec2 od = vec2(0.0);

    for(int i = 0 ; i < SUN_STEPS; i++) {
        vec3 s = p + sunData.sunDir * (t + seg * 0.5);
        od += LocalDensity(s) * seg;
        t += seg;
    }

    return exp(-(BETA_RAY * od.x + BETA_MIE * 1.1 * od.y));
}

vec3 Atmosphere(vec3 rd) {
    vec3 ro = vec3(0.0, R_EARTH, 0.0);

    vec2 tA = RaySphere(ro, rd, R_ATMOS);
    vec2 tE = RaySphere(ro, rd, R_EARTH);

    if(tA.y < 0.0) return vec3(0.0);

    float tMin = max(tA.x, 0.0);
    float tMax = tA.y;

    if(tE.x > 0.0 && tE.x < tMax) tMax = tE.x;
    if(tMax <= tMin) return vec3(0.0);

    vec3 accumR = vec3(0.0);
    vec3 accumM = vec3(0.0);

    vec2 optD = vec2(0.0);

    float cosT = dot(rd, sunData.sunDir);
    float pR = PhaseRay(cosT);
    float pM = PhaseMie(cosT);

    for(int i = 0; i < VIEW_STEPS; i++) {
        float u0 = float(i) / float(VIEW_STEPS);
        float u1 = float(i + 1) / float(VIEW_STEPS);

        float t0 = mix(tMin, tMax, u0);
        float t1 = mix(tMin, tMax, u1);

        float seg = t1 - t0;

        float j = Hash(gl_FragCoord.xy + float(i)) - 0.5;
        float tm = mix(t0, t1, 0.5 + j / float(VIEW_STEPS));

        vec3 s = ro + rd * tm;

        vec2 dens = LocalDensity(s) * seg;
        optD += dens;

        vec3 sunTr = SunTransmittance(s);
        vec3 viewTr = exp(-(BETA_RAY * optD.x + BETA_MIE * 1.1 * optD.y));

        accumR += sunTr * viewTr * dens.x;
        accumM += sunTr * viewTr * dens.y;
    }

    float si = 22.0;
    return si * (BETA_RAY * pR * accumR + BETA_MIE * pM * accumM);
}

vec3 SafeAtmosphere(vec3 dir) {
    vec3 ro = vec3(0.0, R_EARTH, 0.0);
    vec2 tA = RaySphere(ro, dir, R_ATMOS);

    if(tA.y < 0.0) return vec3(0.0);

    if(dir.y >= 0.0) return Atmosphere(dir);

    float below = -dir.y;
    float blend = smoothstep(0.0, 0.25, below);

    vec3 hDir = normalize(vec3(dir.x, abs(dir.y), dir.z));
    vec3 horizon = Atmosphere(hDir);
    
    vec3 ground = horizon * vec3(0.35, 0.30, 0.25) * 0.35;
    return mix(horizon, ground, blend);
}

void main() {
    vec3 dir = normalize(inDir);
    vec3 color = SafeAtmosphere(dir);
    vec3 sunDir = normalize(sunData.sunDir);

    float cosS = dot(dir, sunDir);
    float vis = clamp(sunData.sunHeight * 4.0 + 0.3, 0.0, 1.0);
    float disc = smoothstep(0.9995, 0.99998, cosS);
    float corona = pow(max(cosS, 0.0), 128.0) * 0.3;
    float glow = pow(max(cosS, 0.0), 8.0) * 0.08;
    
    color += vec3(1.0, 0.95, 0.85) * (disc * 2.5 + corona + glow) * vis;

    outColor = vec4(color, 1.0);
}