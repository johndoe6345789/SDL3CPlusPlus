#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

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
    vec3 baseColor = clamp(fragColor * 1.05, 0.0, 1.0);
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
