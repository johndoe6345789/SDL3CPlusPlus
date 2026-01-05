#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(63.1, 157.9))) * 43758.5453123);
}

const vec3 SURFACE_TINT = vec3(0.9, 0.93, 0.8);

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

const vec3 LIGHT_COLOR = vec3(0.92, 0.96, 1.0);
const float LIGHT_INTENSITY = 0.85;
const float AMBIENT_STRENGTH = 0.38;

float calculateAttenuation(float distance) {
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 baseColor = clamp(fragColor * 1.15 * SURFACE_TINT, 0.0, 1.0);
    vec2 gridUv = fragWorldPos.xz * 0.45;
    vec2 grid = abs(fract(gridUv) - 0.5);
    float gridLine = step(0.48, max(grid.x, grid.y));
    float speckle = hash(floor(fragWorldPos.xz * 3.0));
    baseColor *= mix(0.94, 1.04, speckle);
    baseColor *= mix(1.0, 0.84, gridLine);

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
