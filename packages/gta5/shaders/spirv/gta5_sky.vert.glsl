#version 450

// Fullscreen triangle. Three vertices, no vertex buffer: the clip-space
// corner comes from gl_VertexIndex, and the fragment stage turns it back
// into a world-space ray.
//
// Depth is 1.0 so the sky sits at the far plane and the city, drawn
// after with depth test on, occludes it without needing a second pass.

layout(location = 0) out vec2 v_ndc;

void main() {
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    v_ndc = uv * 2.0 - 1.0;
    gl_Position = vec4(v_ndc, 1.0, 1.0);
}
