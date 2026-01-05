#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// Material properties
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
} pc;

// Lighting uniforms
layout(set = 0, binding = 0) uniform sampler2D shadowMap;

// Material parameters (can be extended to use textures)
const vec3 MATERIAL_ALBEDO = vec3(0.8, 0.8, 0.8);
const float MATERIAL_ROUGHNESS = 0.3;
const float MATERIAL_METALLIC = 0.1;

// Atmospheric parameters
const vec3 FOG_COLOR = vec3(0.05, 0.05, 0.08);
const float FOG_DENSITY = 0.003;
const float AMBIENT_STRENGTH = 0.01;  // Much darker ambient

// Light properties
const vec3 LIGHT_COLOR = vec3(1.0, 0.9, 0.6);
const float LIGHT_INTENSITY = 1.2;
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

// PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float calculateShadow(vec3 worldPos) {
    vec4 lightSpacePos = pc.lightViewProj * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - 0.005 > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

vec3 calculateLighting(vec3 worldPos, vec3 normal, vec3 viewDir) {
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, MATERIAL_ALBEDO, MATERIAL_METALLIC);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec3 lightDir = normalize(LIGHT_POSITIONS[i] - worldPos);
        vec3 halfway = normalize(viewDir + lightDir);

        float distance = length(LIGHT_POSITIONS[i] - worldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = LIGHT_COLOR * LIGHT_INTENSITY * attenuation;

        float NDF = DistributionGGX(normal, halfway, MATERIAL_ROUGHNESS);
        float G = GeometrySmith(normal, viewDir, lightDir, MATERIAL_ROUGHNESS);
        vec3 F = fresnelSchlick(max(dot(halfway, viewDir), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - MATERIAL_METALLIC;

        float NdotL = max(dot(normal, lightDir), 0.0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;

        Lo += (kD * MATERIAL_ALBEDO / 3.14159 + specular) * radiance * NdotL;
    }

    return Lo;
}

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(pc.cameraPos - fragWorldPos);

    // Ambient lighting
    vec3 ambient = AMBIENT_STRENGTH * MATERIAL_ALBEDO;

    // Direct lighting with PBR
    vec3 lighting = calculateLighting(fragWorldPos, normal, viewDir);

    // Shadow calculation
    float shadow = calculateShadow(fragWorldPos);
    lighting *= (1.0 - shadow * 0.7);  // Soften shadows

    // Combine lighting
    vec3 finalColor = ambient + lighting;

    // Fog
    float fogFactor = 1.0 - exp(-FOG_DENSITY * length(pc.cameraPos - fragWorldPos));
    finalColor = mix(finalColor, FOG_COLOR, fogFactor);

    // Simple tone mapping
    finalColor = finalColor / (finalColor + vec3(1.0));

    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(finalColor, 1.0);
}