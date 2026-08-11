#include <metal_stdlib>
using namespace metal;

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
};

struct BloomParams {
    float4 params; // x=threshold, y=soft_knee, z=0, w=0
};

fragment float4 main0(
    FragmentInput in [[stage_in]],
    texture2d<float> hdr_texture [[texture(0)]],
    sampler hdr_sampler [[sampler(0)]],
    constant BloomParams& bloom [[buffer(0)]])
{
    float3 color = hdr_texture.sample(hdr_sampler, in.uv).rgb;

    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    float threshold = bloom.params.x;
    float knee = bloom.params.y;

    // Soft knee: smooth quadratic falloff near threshold
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);

    float contribution = max(soft, brightness - threshold);
    contribution /= max(brightness, 0.00001);

    return float4(color * max(contribution, 0.0), 1.0);
}
