#version 450

// Quake 3 BSP vertex shader (minimal).
// Vertex format `position_uv_lmuv_normal` (BspRenderVertex):
//   loc 0: vec3 position
//   loc 1: vec2 diffuse uv
//   loc 2: vec2 lightmap uv (already atlas-remapped by bsp.lightmap_atlas step)
//   loc 3: vec3 world-space normal
// Uniform layout matches sdl3cpp::services::rendering::VertexUniformData
// so the same C++ uniform push call used by the seed pipeline works here.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_modelViewProj;
    mat4 u_model;
    vec4 u_surfaceNormal;   // unused for BSP — per-vertex normal wins
    vec4 u_uvScale;
    vec4 u_cameraPos;
    mat4 u_shadowVP;        // unused for BSP — lightmap already encodes shadowing
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec2 v_lmuv;
layout(location = 2) out vec3 v_worldNormal;
layout(location = 3) out vec3 v_worldPos;
layout(location = 4) out vec3 v_cameraPos;

void main() {
    gl_Position = u_modelViewProj * vec4(a_position, 1.0);
    v_uv = a_uv * u_uvScale.xy;
    v_lmuv = a_lmuv;
    vec4 wp = u_model * vec4(a_position, 1.0);
    v_worldPos = wp.xyz;
    v_worldNormal = mat3(u_model) * a_normal;
    v_cameraPos = u_cameraPos.xyz;
}
