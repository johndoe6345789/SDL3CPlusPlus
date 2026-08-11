#include <metal_stdlib>
using namespace metal;

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
};

fragment float4 main0(FragmentInput in [[stage_in]],
                      texture2d<float> overlayTex [[texture(0)]],
                      sampler overlaySampler [[sampler(0)]]) {
    return overlayTex.sample(overlaySampler, in.uv);
}
