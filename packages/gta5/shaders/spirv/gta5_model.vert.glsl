#version 450

// Streamed GTA V map geometry.
//
// Pairs with the seed textured.frag: the varyings below match its inputs
// exactly, so only the vertex stage is new. That fragment shader lights
// by v_worldNormal, and seed's own vertex shader feeds it a single
// constant from u_surfaceNormal -- which is why building walls came out
// black while flat ground looked right. This one passes the real
// per-vertex normal instead.
//
// Vertex format `position_uv_lmuv_normal` (BspRenderVertex, 40 bytes).
// The lightmap uv is unused here; the format is reused rather than
// adding another one to the engine.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_modelViewProj;
    mat4 u_model;
    vec4 u_surfaceNormal;   // unused here -- the per-vertex normal wins
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

    vec4 wp = u_model * vec4(a_position, 1.0);
    v_worldPos = wp.xyz;
    // Matches bsp.vert: good for the rotations and uniform scales most
    // placements use. A placement with scaleXY != scaleZ would want the
    // inverse-transpose to stay exactly perpendicular.
    v_worldNormal = normalize(mat3(u_model) * a_normal);
    v_cameraPos = u_cameraPos.xyz;
    v_shadowPos = u_shadowVP * wp;
}
