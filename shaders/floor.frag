#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

const vec3 SURFACE_BASE = vec3(0.02, 0.95, 0.72);

const vec3 LIGHT_POSITIONS[8] = vec3[8](
    vec3(13.0, 4.5, 13.0),
    vec3(-13.0, 4.5, 13.0),
    vec3(13.0, 4.5, -13.0),
    vec3(-13.0, 4.5, -13.0),
    vec3(0.0, 4.5, 13.0),
    vec3(0.0, 4.5, -13.0),
    vec3(13.0, 4.5, 0.0),
    vec3(-13.0, 4.5, 0.0)
);

const vec3 LIGHT_COLOR = vec3(1.0, 0.92, 0.7);
const float LIGHT_INTENSITY = 1.0;
const float AMBIENT_STRENGTH = 0.3;

float calculateAttenuation(float distance) {
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 baseColor = SURFACE_BASE;
    float checkerScale = 0.55;
    float cx = step(0.5, fract(fragWorldPos.x * checkerScale));
    float cz = step(0.5, fract(fragWorldPos.z * checkerScale));
    float checker = abs(cx - cz);
    float grit = hash(floor(fragWorldPos.xz * 2.2));
    float pattern = mix(0.82, 1.08, checker) * mix(0.96, 1.04, grit);
    baseColor *= pattern;

    vec3 ambient = AMBIENT_STRENGTH * baseColor;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec3 lightDir = LIGHT_POSITIONS[i] - fragWorldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        float attenuation = calculateAttenuation(distance);
        lighting += LIGHT_COLOR * LIGHT_INTENSITY * attenuation;
    }

    vec3 finalColor = ambient + baseColor * lighting;
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, 1.0);
}
