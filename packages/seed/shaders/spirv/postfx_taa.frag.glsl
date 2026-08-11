#version 450

// Temporal Anti-Aliasing resolve
// Blends current frame with history using neighborhood clamping (UE4/Karis style)

layout(set = 2, binding = 0) uniform sampler2D currentFrame;
layout(set = 2, binding = 1) uniform sampler2D historyFrame;

layout(set = 3, binding = 0) uniform TAAParams {
    vec4 u_params;  // x = blend factor (0.05 = 95% history), y = 1/width, z = 1/height, w = frame count
};

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

// Tonemap for stable neighborhood clamping (Karis 2014)
vec3 tonemap(vec3 c) {
    return c / (1.0 + max(c.r, max(c.g, c.b)));
}

vec3 untonemap(vec3 c) {
    return c / (1.0 - max(c.r, max(c.g, c.b)));
}

void main() {
    vec2 texelSize = u_params.yz;
    float blendFactor = u_params.x;
    float frameCount = u_params.w;

    vec3 current = texture(currentFrame, v_uv).rgb;

    // First frame: no history, just output current
    if (frameCount < 1.5) {
        o_color = vec4(current, 1.0);
        return;
    }

    // Sample 3x3 neighborhood of current frame for clamping
    vec3 nMin = current;
    vec3 nMax = current;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            if (x == 0 && y == 0) continue;
            vec3 s = texture(currentFrame, v_uv + vec2(float(x), float(y)) * texelSize).rgb;
            nMin = min(nMin, s);
            nMax = max(nMax, s);
        }
    }

    // Slightly expand the bounding box to reduce flickering
    vec3 nCenter = (nMin + nMax) * 0.5;
    vec3 nExtent = (nMax - nMin) * 0.5;
    nMin = nCenter - nExtent * 1.25;
    nMax = nCenter + nExtent * 1.25;

    // Sample history and clamp to neighborhood (prevents ghosting)
    vec3 history = texture(historyFrame, v_uv).rgb;
    vec3 historyTM = tonemap(history);
    vec3 clampedTM = clamp(historyTM, tonemap(nMin), tonemap(nMax));
    vec3 clamped = untonemap(clampedTM);

    // Blend: low factor = more history = smoother but more ghosting
    vec3 result = mix(clamped, current, blendFactor);

    o_color = vec4(result, 1.0);
}
