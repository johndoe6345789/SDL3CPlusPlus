#version 450

layout(location = 0) out vec2 v_uv;

void main() {
    // Fullscreen triangle: 3 vertices covering [-1,1] clip space
    v_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);
    // Flip Y for Vulkan's top-left origin
    v_uv.y = 1.0 - v_uv.y;
}
