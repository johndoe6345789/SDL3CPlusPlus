#version 450

// Checkpoint arrow fragment. The arrow mesh stores each face's shade in the
// u coordinate, so faces read as lit planes without any light uniforms.

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 o_color;

const vec3 kArrowDark = vec3(0.85, 0.20, 0.04);
const vec3 kArrowBright = vec3(1.00, 0.82, 0.12);

void main() {
    o_color = vec4(mix(kArrowDark, kArrowBright, v_uv.x), 1.0);
}
