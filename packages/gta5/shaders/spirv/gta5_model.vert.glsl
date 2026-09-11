#version 450

// Streamed GTA V map geometry, drawn instanced.
//
// Every copy of an archetype in view is one draw: gta5.tiles.cull writes
// the visible instances' model matrices, grouped by archetype, into one
// storage buffer each frame, and a draw's first_instance says where its
// group starts. One call per archetype material replaced one per
// placement, and the draw loop was the whole frame.
//
// Vertex format `position_uv_lmuv_normal` (BspRenderVertex, 40 bytes).
// The varyings match gta5_model.frag.

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
layout(location = 1) out vec3 v_worldNormal;
layout(location = 2) out vec3 v_worldPos;
layout(location = 3) out vec3 v_cameraPos;
layout(location = 4) out vec4 v_shadowPos;
layout(location = 5) out vec4 v_blend;  // colour 1, for terrain

void main() {
    // Under Vulkan gl_InstanceIndex includes the draw's first_instance,
    // which is the group's first matrix: no per-draw uniform. This
    // package ships SPIR-V only; D3D would need a base pushed instead.
    mat4 model = u_models[gl_InstanceIndex];
    vec4 wp = model * vec4(a_position, 1.0);
    gl_Position = u_viewProj * wp;
    v_uv = a_uv;
    v_worldPos = wp.xyz;
    // Good for the rotations and uniform scales most placements use.
    v_worldNormal = normalize(mat3(model) * a_normal);
    v_cameraPos = u_cameraPos.xyz;
    v_shadowPos = u_shadowVP * wp;
    // Colour 1 comes packed two bytes to a float in the lightmap uv,
    // as r + 256 g and b + 256 a (see gta5_vertex_layout).
    v_blend = vec4(mod(a_lmuv.x, 256.0), floor(a_lmuv.x / 256.0),
                   mod(a_lmuv.y, 256.0), floor(a_lmuv.y / 256.0)) /
              255.0;
}
