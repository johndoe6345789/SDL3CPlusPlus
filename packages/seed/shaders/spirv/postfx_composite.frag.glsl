#version 450

layout(set = 2, binding = 0) uniform sampler2D hdr_texture;
layout(set = 2, binding = 1) uniform sampler2D ssao_texture;
layout(set = 2, binding = 2) uniform sampler2D bloom_texture;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

// ACES filmic tonemapping (Narkowicz fit)
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(hdr_texture, v_uv).rgb;

    // Apply SSAO occlusion
    float ao = texture(ssao_texture, v_uv).r;
    hdr *= ao;

    // Additive bloom
    vec3 bloom = texture(bloom_texture, v_uv).rgb;
    hdr += bloom;

    // ACES filmic tonemapping
    vec3 mapped = ACESFilm(hdr);

    // sRGB gamma correction
    vec3 result = pow(mapped, vec3(1.0 / 2.2));

    o_color = vec4(result, 1.0);
}
