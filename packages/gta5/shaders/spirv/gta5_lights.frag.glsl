#version 450

// A distant light: a soft round glow, brightest at the centre.

layout(location = 0) in vec2 v_corner;
layout(location = 1) in vec4 v_colour;

layout(location = 0) out vec4 o_color;

void main() {
    float r2 = dot(v_corner, v_corner);
    if (r2 > 1.0) discard;
    float glow = exp(-r2 * 5.0);
    o_color = vec4(v_colour.rgb * (1.0 + 2.0 * glow),
                   clamp(glow * v_colour.a * 3.0, 0.0, 1.0));
}
