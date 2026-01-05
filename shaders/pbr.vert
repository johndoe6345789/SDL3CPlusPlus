#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
} pc;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    gl_Position = pc.proj * pc.view * worldPos;

    fragWorldPos = worldPos.xyz;
    fragNormal = mat3(pc.model) * inNormal;
    fragTexCoord = inTexCoord;
    fragColor = vec3(0.7, 0.7, 0.7);  // Default color, can be from vertex color or texture
}