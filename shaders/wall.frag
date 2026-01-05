#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(91.7, 127.3))) * 43758.5453123);
}

const vec3 SURFACE_TINT = vec3(1.0, 0.32, 0.08);

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

const vec3 LIGHT_COLOR = vec3(1.0, 0.88, 0.6);
const float LIGHT_INTENSITY = 0.95;
const float AMBIENT_STRENGTH = 0.24;

float calculateAttenuation(float distance) {
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 baseColor = clamp(fragColor * 1.05 * SURFACE_TINT, 0.0, 1.0);
    float axisSelector = step(abs(fragWorldPos.z), abs(fragWorldPos.x));
    float coord = mix(fragWorldPos.z, fragWorldPos.x, axisSelector);
    float plankScale = 0.4;
    float plank = abs(fract(coord * plankScale) - 0.5);
    float groove = smoothstep(0.46, 0.5, plank);
    float grain = hash(floor(vec2(coord * 1.2, fragWorldPos.y * 2.0)));
    baseColor *= mix(0.92, 1.05, grain);
    baseColor *= mix(1.0, 0.78, groove);

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
