#include <metal_stdlib>
using namespace metal;

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
};

struct BlurParams {
    float4 direction; // xy = texel-scaled blur direction
};

fragment float4 main0(
    FragmentInput in [[stage_in]],
    texture2d<float> source [[texture(0)]],
    sampler src_sampler [[sampler(0)]],
    constant BlurParams& blur [[buffer(0)]])
{
    float2 dir = blur.direction.xy;

    // 9-tap Gaussian (sigma ~= 4, weights sum to 1.0)
    const float weight[5] = {0.227027027, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162};

    float3 result = source.sample(src_sampler, in.uv).rgb * weight[0];

    for (int i = 1; i < 5; ++i) {
        float2 offset = dir * float(i);
        result += source.sample(src_sampler, in.uv + offset).rgb * weight[i];
        result += source.sample(src_sampler, in.uv - offset).rgb * weight[i];
    }

    return float4(result, 1.0);
}
