#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct VertexOutput {
    float4 position [[position]];
    float2 uv;
};

vertex VertexOutput main0(VertexInput in [[stage_in]]) {
    VertexOutput out;
    out.position = float4(in.position, 1.0);
    out.uv = in.uv;
    return out;
}
