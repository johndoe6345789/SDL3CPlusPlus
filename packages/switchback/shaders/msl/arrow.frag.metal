#include <metal_stdlib>
using namespace metal;

// Checkpoint arrow fragment. The arrow mesh stores each face's shade in the
// u coordinate, so faces read as lit planes without any light uniforms.

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
    float3 worldNormal;
    float3 worldPos;
    float3 cameraPos;
    float4 shadowPos;
};

constant float3 kArrowDark = float3(0.85, 0.20, 0.04);
constant float3 kArrowBright = float3(1.00, 0.82, 0.12);

fragment float4 main0(FragmentInput in [[stage_in]])
{
    float3 colour = mix(kArrowDark, kArrowBright, in.uv.x);
    return float4(colour, 1.0);
}
