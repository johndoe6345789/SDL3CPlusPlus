#version 450

// Depth only, except that a cutout -- a leaf card -- casts the shape in
// its alpha rather than a rectangle.

layout(set = 2, binding = 0) uniform sampler2D albedoTex;

layout(set = 3, binding = 0) uniform ShadowUniforms {
    vec4 u_surface;  // a = alpha discard threshold, 0 = solid
};

layout(location = 0) in vec2 v_uv;

void main() {
    if (u_surface.a > 0.0 && texture(albedoTex, v_uv).a < u_surface.a) {
        discard;
    }
}
