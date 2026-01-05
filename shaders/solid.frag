#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

// Lantern positions (8 lights)
const vec3 LIGHT_POSITIONS[8] = vec3[8](
    vec3(13.0, 4.5, 13.0),    // Corner
    vec3(-13.0, 4.5, 13.0),   // Corner
    vec3(13.0, 4.5, -13.0),   // Corner
    vec3(-13.0, 4.5, -13.0),  // Corner
    vec3(0.0, 4.5, 13.0),     // Wall midpoint
    vec3(0.0, 4.5, -13.0),    // Wall midpoint
    vec3(13.0, 4.5, 0.0),     // Wall midpoint
    vec3(-13.0, 4.5, 0.0)     // Wall midpoint
);

const vec3 LIGHT_COLOR = vec3(1.0, 0.9, 0.6);  // Warm lantern color
const float LIGHT_INTENSITY = 0.9;
const float AMBIENT_STRENGTH = 0.22;  // Boost ambient to preserve surface color

float calculateAttenuation(float distance) {
    // Quadratic attenuation: 1 / (constant + linear*d + quadratic*d^2)
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 ambient = AMBIENT_STRENGTH * fragColor;
    vec3 lighting = vec3(0.0);

    // Calculate contribution from each lantern
    for (int i = 0; i < 8; i++) {
        vec3 lightDir = LIGHT_POSITIONS[i] - fragWorldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        // Distance attenuation
        float attenuation = calculateAttenuation(distance);

        // Add light contribution
        lighting += LIGHT_COLOR * LIGHT_INTENSITY * attenuation;
    }

    // Combine ambient and dynamic lighting with surface color
    vec3 finalColor = ambient + fragColor * lighting;

    // Clamp to prevent over-bright areas
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, 1.0);
}
