#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
} pushConstants;

void main() {
    fragColor = inColor;
    vec4 worldPos = pushConstants.model * vec4(inPos, 1.0);
    fragWorldPos = worldPos.xyz;
    gl_Position = pushConstants.viewProj * worldPos;
}
