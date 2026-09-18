#version 450

// FS2024's water, a tile at a time: flat outlines in the tile's own
// space, placed in engine space by u_originOffset like the ground, so
// gta5_water.frag sees true world positions for its swell and haze.
// Vertex format position_uv_lmuv_normal.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform WaterVertex {
    mat4 u_viewProj;
    vec4 u_originOffset;  // xyz: where this tile's local origin sits
};

layout(location = 0) out vec3 v_worldPos;

void main() {
    vec3 world = a_position + u_originOffset.xyz;
    gl_Position = u_viewProj * vec4(world, 1.0);
    v_worldPos = world;
}
