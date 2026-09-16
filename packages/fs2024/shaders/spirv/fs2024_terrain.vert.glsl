#version 450

// FS2024 ground: the heightfield baked by python/fs2024/bake_terrain.py,
// drawn in blocks. Positions are already world space, so there is no
// model matrix; uv spans the whole field once and reads the ground map.
//
// Vertex format `position_uv_lmuv_normal` (BspRenderVertex, 40 bytes).
// The lightmap uv is unused.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_viewProj;
    vec4 u_cameraPos;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_worldPos;
layout(location = 3) out vec3 v_cameraPos;

void main() {
    gl_Position = u_viewProj * vec4(a_position, 1.0);
    v_uv = a_uv;
    v_normal = a_normal;
    v_worldPos = a_position;
    v_cameraPos = u_cameraPos.xyz;
}
