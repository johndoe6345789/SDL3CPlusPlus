#include <metal_stdlib>
using namespace metal;

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
};

// ACES filmic tonemapping (Narkowicz fit)
float3 ACESFilm(float3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

fragment float4 main0(
    FragmentInput in [[stage_in]],
    texture2d<float> hdr_texture [[texture(0)]],
    sampler hdr_sampler [[sampler(0)]],
    texture2d<float> ssao_texture [[texture(1)]],
    sampler ssao_sampler [[sampler(1)]],
    texture2d<float> bloom_texture [[texture(2)]],
    sampler bloom_sampler [[sampler(2)]])
{
    float3 hdr = hdr_texture.sample(hdr_sampler, in.uv).rgb;

    // Apply SSAO occlusion
    float ao = ssao_texture.sample(ssao_sampler, in.uv).r;
    hdr *= ao;

    // Additive bloom
    float3 bloom = bloom_texture.sample(bloom_sampler, in.uv).rgb;
    hdr += bloom;

    // ACES filmic tonemapping
    float3 mapped = ACESFilm(hdr);

    // sRGB gamma correction
    float3 result = pow(mapped, float3(1.0 / 2.2));

    return float4(result, 1.0);
}
