#version 450

// FS2024's own trees and shrubs: crossed billboards, baked already in
// tile-local space (see fs2024_vegetation_quad.cpp), placed in engine
// space the same way the ground is. Vertex format
// position_uv_lmuv_normal; the lightmap u carries the atlas layer
// (a variation's own textureIndex) as a whole float.

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
layout(location = 4) out float v_layer;

void main() {
    vec3 world = a_position + u_originOffset.xyz;
    gl_Position = u_viewProj * vec4(world, 1.0);
    v_uv = a_uv;
    v_normal = a_normal;
    v_worldPos = world;
    v_cameraPos = u_cameraPos.xyz;
    v_layer = a_lmuv.x;
}
