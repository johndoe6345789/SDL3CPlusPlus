#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

layout(std140, binding = 0) uniform Uniforms {
    mat4 u_modelViewProj;
};

layout(location = 0) out vec4 v_color;

void main() {
    gl_Position = u_modelViewProj * vec4(position, 1.0);
    v_color = vec4(color, 1.0);
}
