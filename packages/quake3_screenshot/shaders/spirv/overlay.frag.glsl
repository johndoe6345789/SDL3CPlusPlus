#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler2D font_tex;

void main() {
    out_color = texture(font_tex, v_uv);
}
