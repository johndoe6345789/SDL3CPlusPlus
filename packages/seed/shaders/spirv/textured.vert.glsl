#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_modelViewProj;
    mat4 u_model;
    vec4 u_surfaceNormal;
    vec4 u_uvScale;
    vec4 u_cameraPos;
    mat4 u_shadowVP;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_worldNormal;
layout(location = 2) out vec3 v_worldPos;
layout(location = 3) out vec3 v_cameraPos;
layout(location = 4) out vec4 v_shadowPos;

void main() {
    gl_Position = u_modelViewProj * vec4(a_position, 1.0);
    v_uv = a_uv * u_uvScale.xy;
    v_worldNormal = u_surfaceNormal.xyz;
    vec4 wp = u_model * vec4(a_position, 1.0);
    v_worldPos = wp.xyz;
    v_cameraPos = u_cameraPos.xyz;
    v_shadowPos = u_shadowVP * wp;
}
