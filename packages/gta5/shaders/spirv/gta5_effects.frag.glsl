#version 450

// The sprite strip holds a puff, a flash, smoke and a scorch mark side
// by side; the vertex carries which, in its u, and the colour and
// brightness to draw it with.

layout(set = 2, binding = 0) uniform sampler2D sprites;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_colour;

layout(location = 0) out vec4 o_color;

void main() {
    vec4 sprite = texture(sprites, v_uv);
    // Fire and flashes are light: their colour is added, so the frame's
    // exposure lifts them the way the sky and the lamps are lifted.
    o_color = vec4(sprite.rgb * v_colour.rgb, sprite.a * v_colour.a);
}
