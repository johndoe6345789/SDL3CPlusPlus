#version 450

// One BL4 static mesh instance, drawn with a per-draw model matrix (see
// bl4_model_draw_step.cpp -- one draw per resident placement, not yet
// instanced like gta5_model.vert, since a first walkable slice matters
// more than draw-call count here).
//
// Vertex format `position_uv_lmuv_normal` (BspRenderVertex, 40 bytes).
// The lightmap uv is unused: BL4 meshes carry no baked lightmap here.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_viewProj;
    mat4 u_model;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_worldNormal;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    gl_Position = u_viewProj * worldPos;
    v_uv = a_uv;
    v_worldNormal = normalize(mat3(u_model) * a_normal);
}
