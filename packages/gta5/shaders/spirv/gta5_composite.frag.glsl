#version 450

// Scene target to swapchain: a supersample resolve with contrast-
// adaptive sharpening when the scene was rendered larger than the
// window, and FXAA when it was not.
//
// Supersampled (frame.gpu.begin_offscreen render_scale 2), every window
// pixel is the average of the scene texels under it: distant wires,
// railings and foliage cutouts resolve to steady partial coverage
// instead of crawling, and nothing is blurred to get there. A light
// sharpen, weighted down where the neighbourhood is already contrasty
// so edges do not ring, then gives back the crispness a box filter
// takes. FXAA is the fallback at render_scale 1: it blurs along edges,
// which reads as soft in the distance.
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
const float SHARPNESS = 0.6;      // 0 = gentle, 1 = strongest

float luma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

// ACES filmic (Narkowicz fit), as the seed composite uses.
vec3 tonemap(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14),
                 0.0, 1.0);
}

vec3 tapped(vec2 uv) {
    return tonemap(texture(hdr_texture, uv).rgb);
}

// One window pixel: four bilinear taps a quarter of its footprint from
// the centre. At render_scale 2 each lands on one texel's centre, so
// this is the exact 2x2 box; tone mapped first, so a bright sky does not
// swallow the wire in front of it.
vec3 resolved(vec2 uv, vec2 pixel) {
    vec2 q = pixel * 0.25;
    return 0.25 * (tapped(uv + vec2(-q.x, -q.y)) +
                   tapped(uv + vec2(q.x, -q.y)) +
                   tapped(uv + vec2(-q.x, q.y)) +
                   tapped(uv + vec2(q.x, q.y)));
}

vec3 supersampled(vec2 pixel) {
    vec3 c = resolved(v_uv, pixel);
    vec3 n = resolved(v_uv + vec2(0.0, -pixel.y), pixel);
    vec3 s = resolved(v_uv + vec2(0.0, pixel.y), pixel);
    vec3 w = resolved(v_uv + vec2(-pixel.x, 0.0), pixel);
    vec3 e = resolved(v_uv + vec2(pixel.x, 0.0), pixel);
    // Contrast-adaptive: the headroom left between the neighbourhood's
    // extremes sets how hard a negative-lobed cross may push.
    vec3 lo = min(c, min(min(n, s), min(w, e)));
    vec3 hi = max(c, max(max(n, s), max(w, e)));
    vec3 amount = sqrt(clamp(min(lo, 1.0 - hi) / max(hi, vec3(1e-4)),
                             0.0, 1.0));
    vec3 lobe = amount * (-1.0 / mix(8.0, 5.0, SHARPNESS));
    return clamp((c + (n + s + w + e) * lobe) / (1.0 + 4.0 * lobe), 0.0,
                 1.0);
}

vec3 fxaa(vec2 texel) {
    // Tone map before measuring edges. The scene target is HDR, and a
    // bright sky against a dark wall is a huge linear step but a modest
    // displayed one.
    vec3 mid = tapped(v_uv);
    float lNW = luma(tapped(v_uv + vec2(-1.0, -1.0) * texel));
    float lNE = luma(tapped(v_uv + vec2(1.0, -1.0) * texel));
    float lSW = luma(tapped(v_uv + vec2(-1.0, 1.0) * texel));
    float lSE = luma(tapped(v_uv + vec2(1.0, 1.0) * texel));
    float lM = luma(mid);
    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));
    if (lMax - lMin < max(EDGE_MIN, lMax * EDGE_SCALE)) return mid;

    // Blur along the edge, perpendicular to the corners' luma gradient.
    vec2 dir = normalize(vec2(-((lNW + lNE) - (lSW + lSE)),
                              ((lNW + lSW) - (lNE + lSE))) + vec2(1e-6));
    float scale = 1.0 / min(abs(dir.x), abs(dir.y));
    dir = clamp(dir * scale, -SPAN_MAX, SPAN_MAX) * texel;
    vec3 inner = 0.5 * (tapped(v_uv + dir * (1.0 / 3.0 - 0.5)) +
                        tapped(v_uv + dir * (2.0 / 3.0 - 0.5)));
    vec3 outer = 0.5 * inner +
                 0.25 * (tapped(v_uv - dir * 0.5) + tapped(v_uv + dir * 0.5));
    // The wide tap reaches past a thin feature; fall back when it leaves
    // the neighbourhood.
    float lOuter = luma(outer);
    return (lOuter < lMin || lOuter > lMax) ? inner : outer;
}

void main() {
    vec2 texel = 1.0 / vec2(textureSize(hdr_texture, 0));
    // One window pixel's extent in uv; the scene target is larger than
    // the window when it spans more than one texel.
    vec2 pixel = vec2(abs(dFdx(v_uv.x)), abs(dFdy(v_uv.y)));
    vec3 result = (pixel.x > texel.x * 1.25) ? supersampled(pixel)
                                             : fxaa(texel);
    o_color = vec4(pow(result, vec3(1.0 / 2.2)), 1.0);
}
