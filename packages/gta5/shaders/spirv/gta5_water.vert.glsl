#version 450

// GTA V's water, from water.xml: flat quads at their own heights, in
// world space already. Vertex format position_uv_lmuv_normal.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform WaterVertex {
    mat4 u_viewProj;
};

layout(location = 0) out vec3 v_worldPos;

void main() {
    gl_Position = u_viewProj * vec4(a_position, 1.0);
    v_worldPos = a_position;
}
