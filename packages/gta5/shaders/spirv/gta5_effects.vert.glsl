#version 450

// Muzzle flashes, tracers, dust, fire, smoke and scorch marks. Each
// quad's corners are already in world space -- turned to face the
// camera, stretched along a tracer, or laid on the surface it marks --
// so this only projects them. Vertex format position_uv_lmuv_normal:
// the corner, its place in the sprite strip, the brightness in lm_u,
// and the colour in the normal.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform EffectUniforms {
    mat4 u_viewProj;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_colour;

void main() {
    gl_Position = u_viewProj * vec4(a_position, 1.0);
    v_uv = a_uv;
    v_colour = vec4(a_normal, a_lmuv.x);
}
