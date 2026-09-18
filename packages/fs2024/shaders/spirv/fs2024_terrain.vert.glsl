#version 450

// FS2024 ground and buildings, one tile at a time. Positions are in the
// tile's own local space and u_originOffset places the tile in engine
// space -- no model matrix, just a translation, and never a float that
// has to hold a coordinate from the far side of a long flight. uv spans
// the tile once and reads its land-class map.
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
    vec4 u_originOffset;  // xyz: where this tile's local origin sits
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_worldPos;
layout(location = 3) out vec3 v_cameraPos;

void main() {
    vec3 world = a_position + u_originOffset.xyz;
    gl_Position = u_viewProj * vec4(world, 1.0);
    v_uv = a_uv;
    v_normal = a_normal;
    v_worldPos = world;
    v_cameraPos = u_cameraPos.xyz;
}
