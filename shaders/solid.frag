#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

void main() {
    // Use vertex color directly for solid colors
    outColor = vec4(fragColor, 1.0);
}
