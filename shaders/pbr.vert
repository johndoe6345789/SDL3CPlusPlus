#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;  // Color instead of normal
layout(location = 2) in vec2 inTexCoord;  // Not used for now

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    gl_Position = pc.proj * pc.view * worldPos;

    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(pc.model) * vec3(0.0, 0.0, 1.0));  // Simple normal for flat shading
    fragTexCoord = vec2(0.0, 0.0);  // Not used
    fragColor = inColor;  // Use vertex color
}