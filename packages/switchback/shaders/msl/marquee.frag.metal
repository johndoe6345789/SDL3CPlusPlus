#include <metal_stdlib>
using namespace metal;

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
    float3 worldNormal;
    float3 worldPos;
    float3 cameraPos;
    float4 shadowPos;
};

fragment float4 main0(FragmentInput in [[stage_in]]) {
    const float3 red = float3(0.86, 0.10, 0.08);
    const float3 white = float3(0.96, 0.96, 0.92);
    const float3 black = float3(0.04, 0.04, 0.05);
    const float shade = 0.45 + 0.55 * in.uv.x;
    float3 base;
    if (in.uv.y > 0.5) {
        const float cell = fmod(floor(in.worldPos.x / 1.2) +
                                floor(in.worldPos.y / 1.2), 2.0);
        base = mix(black, white, cell);
    } else {
        const float band = step(0.5, fract(in.worldPos.y / 1.2));
        base = mix(red, white, band);
    }
    return float4(base * shade + 0.08, 1.0);
}
