#include <metal_stdlib>
using namespace metal;

// Temporal Anti-Aliasing resolve
// Blends current frame with history using neighborhood clamping (UE4/Karis style)

struct TAAParams {
    float4 u_params;  // x = blend factor (0.05 = 95% history), y = 1/width, z = 1/height, w = frame count
};

struct FragIn {
    float2 uv [[user(uv)]];
};

// Tonemap for stable neighborhood clamping (Karis 2014)
inline float3 tonemap(float3 c) {
    return c / (1.0f + max(c.r, max(c.g, c.b)));
}

inline float3 untonemap(float3 c) {
    return c / (1.0f - max(c.r, max(c.g, c.b)));
}

fragment float4 main0(
    FragIn in [[stage_in]],
    texture2d<float> currentFrame  [[texture(0)]],
    texture2d<float> historyFrame  [[texture(1)]],
    sampler currentSampler         [[sampler(0)]],
    sampler historySampler         [[sampler(1)]],
    constant TAAParams& params     [[buffer(0)]])
{
    float2 texelSize   = params.u_params.yz;
    float  blendFactor = params.u_params.x;
    float  frameCount  = params.u_params.w;
    float2 uv = in.uv;

    float3 current = currentFrame.sample(currentSampler, uv).rgb;

    // First frame: no history, just output current
    if (frameCount < 1.5f) {
        return float4(current, 1.0f);
    }

    // Sample 3x3 neighbourhood of current frame for clamping
    float3 nMin = current;
    float3 nMax = current;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            if (x == 0 && y == 0) continue;
            float3 s = currentFrame.sample(currentSampler,
                uv + float2(float(x), float(y)) * texelSize).rgb;
            nMin = min(nMin, s);
            nMax = max(nMax, s);
        }
    }

    // Slightly expand the bounding box to reduce flickering
    float3 nCenter = (nMin + nMax) * 0.5f;
    float3 nExtent = (nMax - nMin) * 0.5f;
    nMin = nCenter - nExtent * 1.25f;
    nMax = nCenter + nExtent * 1.25f;

    // Sample history and clamp to neighbourhood (prevents ghosting)
    float3 history   = historyFrame.sample(historySampler, uv).rgb;
    float3 historyTM = tonemap(history);
    float3 clampedTM = clamp(historyTM, tonemap(nMin), tonemap(nMax));
    float3 clamped   = untonemap(clampedTM);

    // Blend: low factor = more history = smoother but more ghosting
    float3 result = mix(clamped, current, blendFactor);

    return float4(result, 1.0f);
}
