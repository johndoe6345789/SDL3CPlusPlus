#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics (ignored by basic shaders)
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
} pushConstants;

void main() {
    fragColor = inColor;
    vec4 worldPos = pushConstants.model * vec4(inPos, 1.0);
    fragWorldPos = worldPos.xyz;
    gl_Position = pushConstants.viewProj * worldPos;
}
