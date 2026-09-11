#version 450

// Scene target to swapchain, with FXAA.
//
// Not MSAA: a city is mostly alpha-cutout foliage, railings and wires,
// and MSAA does nothing for an edge a discard made -- it only smooths
// edges between triangles. Not TAA either: the engine has a history
// blend but no motion vectors, so a car at speed would smear. FXAA
// works on the finished image, so it catches the cutout edges and the
// specular sparkle equally, at one pass and no history.
//
// Same three bindings as the seed composite so the existing
// postfx.composite_draw step drives it unchanged; ssao and bloom are
// bound to the scene target when they are not in the chain, and this
// package does not run them, so they are not read.

layout(set = 2, binding = 0) uniform sampler2D hdr_texture;
layout(set = 2, binding = 1) uniform sampler2D ssao_texture;
layout(set = 2, binding = 2) uniform sampler2D bloom_texture;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

const float EDGE_MIN = 0.03125;   // below this, the pixel is flat
const float EDGE_SCALE = 0.125;   // edge threshold as a share of the max
const float SPAN_MAX = 8.0;       // how far the blur may reach, in texels

float luma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

// ACES filmic (Narkowicz fit), as the seed composite uses.
vec3 tonemap(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14),
                 0.0, 1.0);
}

void main() {
    vec2 texel = 1.0 / vec2(textureSize(hdr_texture, 0));

    // Tone map before measuring edges. The scene target is HDR, and a
    // bright sky against a dark wall is a huge linear step but a modest
    // displayed one -- thresholding the linear values makes FXAA chase
    // contrast the viewer cannot see.
    vec3 mid = tonemap(texture(hdr_texture, v_uv).rgb);
    float lNW = luma(tonemap(texture(hdr_texture,
                                     v_uv + vec2(-1.0, -1.0) * texel).rgb));
    float lNE = luma(tonemap(texture(hdr_texture,
                                     v_uv + vec2(1.0, -1.0) * texel).rgb));
    float lSW = luma(tonemap(texture(hdr_texture,
                                     v_uv + vec2(-1.0, 1.0) * texel).rgb));
    float lSE = luma(tonemap(texture(hdr_texture,
                                     v_uv + vec2(1.0, 1.0) * texel).rgb));
    float lM = luma(mid);

    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));
    vec3 result = mid;

    if (lMax - lMin >= max(EDGE_MIN, lMax * EDGE_SCALE)) {
        // Blur along the edge, which is perpendicular to the luma
        // gradient the four corners give.
        vec2 dir = normalize(vec2(-((lNW + lNE) - (lSW + lSE)),
                                  ((lNW + lSW) - (lNE + lSE))) +
                             vec2(1e-6));
        float scale = 1.0 / min(abs(dir.x), abs(dir.y));
        dir = clamp(dir * scale, -SPAN_MAX, SPAN_MAX) * texel;

        vec3 inner = 0.5 * (tonemap(texture(hdr_texture,
                                            v_uv + dir * (1.0 / 3.0 - 0.5))
                                        .rgb) +
                            tonemap(texture(hdr_texture,
                                            v_uv + dir * (2.0 / 3.0 - 0.5))
                                        .rgb));
        vec3 outer = 0.5 * inner +
                     0.25 * (tonemap(texture(hdr_texture, v_uv - dir * 0.5)
                                         .rgb) +
                             tonemap(texture(hdr_texture, v_uv + dir * 0.5)
                                         .rgb));
        // The wide tap reaches past the edge on a thin feature, where it
        // pulls in a colour that does not belong; fall back when it
        // leaves the neighbourhood.
        float lOuter = luma(outer);
        result = (lOuter < lMin || lOuter > lMax) ? inner : outer;
    }

    o_color = vec4(pow(result, vec3(1.0 / 2.2)), 1.0);
}
