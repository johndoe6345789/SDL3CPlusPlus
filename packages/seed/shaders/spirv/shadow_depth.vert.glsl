#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv; // present in vertex buffer but unused

layout(set = 1, binding = 0) uniform ShadowUniforms {
    mat4 u_lightVP;
    mat4 u_model;
};

void main() {
    gl_Position = u_lightVP * u_model * vec4(a_position, 1.0);
}
