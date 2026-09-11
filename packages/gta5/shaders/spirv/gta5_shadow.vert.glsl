#version 450

// The sun's shadow map: the same instanced geometry as gta5_model.vert,
// seen from the sun. u_viewProj is the light's view and projection.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(std430, set = 0, binding = 0) readonly buffer Instances {
    mat4 u_models[];
};

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_viewProj;
    mat4 u_shadowVP;
    vec4 u_cameraPos;
};

layout(location = 0) out vec2 v_uv;

void main() {
    gl_Position = u_viewProj * (u_models[gl_InstanceIndex] *
                                vec4(a_position, 1.0));
    v_uv = a_uv;
}
