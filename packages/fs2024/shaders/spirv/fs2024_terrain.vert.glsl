#version 450

// FS2024 ground, buildings and landmarks. Positions are local -- a
// tile's own space, or a landmark model's -- and u_model then
// u_originOffset place them in engine space: never a float that has to
// hold a coordinate from the far side of a long flight. Tiles draw with
// an identity u_model; a landmark's turns and scales its one shared
// mesh to each place FS2024 puts it.
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
    mat4 u_model;         // local to tile space: rotation, scale, offset
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_worldPos;
layout(location = 3) out vec3 v_cameraPos;

void main() {
    vec3 world = (u_model * vec4(a_position, 1.0)).xyz + u_originOffset.xyz;
    gl_Position = u_viewProj * vec4(world, 1.0);
    v_uv = a_uv;
    v_normal = mat3(u_model) * a_normal;
    v_worldPos = world;
    v_cameraPos = u_cameraPos.xyz;
}
