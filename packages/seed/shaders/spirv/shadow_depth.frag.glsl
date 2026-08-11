#version 450

// Minimal fragment shader for depth-only pass
layout(location = 0) out vec4 o_color;

void main() {
    o_color = vec4(0.0);
}
