#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
} pushConstants;

void main() {
    fragColor = inColor;
    gl_Position = pushConstants.viewProj * pushConstants.model * vec4(inPos, 1.0);
}
