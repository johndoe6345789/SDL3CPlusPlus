#version 450

// One BL4 static mesh archetype, drawn instanced: bl4.models.draw culls
// the resident tiles to the view and writes every visible copy's model
// matrix into one storage buffer, grouped by archetype, and each draw's
// first_instance says where its group starts. One call per archetype
// submesh replaced one per placement, which left the GPU idle while the
// CPU issued tens of thousands of calls across the full map.
//
// Vertex format `position_uv_lmuv_normal` (BspRenderVertex, 40 bytes).
// The lightmap uv is unused: BL4 meshes carry no baked lightmap here.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(std430, set = 0, binding = 0) readonly buffer Instances {
    mat4 u_models[];
};

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_viewProj;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_worldNormal;

void main() {
    // Under Vulkan gl_InstanceIndex includes the draw's first_instance.
    mat4 model = u_models[gl_InstanceIndex];
    vec4 worldPos = model * vec4(a_position, 1.0);
    gl_Position = u_viewProj * worldPos;
    v_uv = a_uv;
    v_worldNormal = normalize(mat3(model) * a_normal);
}
