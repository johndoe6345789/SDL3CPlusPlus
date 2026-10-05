#version 450

// Episode I Racer geometry, as decoded from the game's N64-style display
// lists. Vertex format `position_uv_lmuv_normal` (40 bytes), reused:
//   lmuv.x  = the vertex alpha
//   normal  = the vertex colour (rgb), which the N64 data uses for its
//             baked lighting instead of normals.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_colour;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_viewProj;
    mat4 u_model;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_colour;
layout(location = 2) out vec3 v_worldPos;

void main() {
    vec4 world = u_model * vec4(a_position, 1.0);
    gl_Position = u_viewProj * world;
    v_uv = a_uv;
    v_colour = vec4(a_colour, a_lmuv.x);
    v_worldPos = world.xyz;
}
