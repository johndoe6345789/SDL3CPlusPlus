#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
// layout(location = 2) in vec2 inTexCoord;  // Not used

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
    gl_Position = pc.lightViewProj * pc.model * vec4(inPosition, 1.0);
}