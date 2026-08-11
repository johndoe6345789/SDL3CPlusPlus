#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float4x4 u_modelViewProj;
};

struct VertexInput {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
};

struct VertexOutput {
    float4 position [[position]];
    float4 color;
};

vertex VertexOutput main0(
    VertexInput in [[stage_in]],
    constant Uniforms& uniforms [[buffer(0)]])
{
    VertexOutput out;
    out.position = uniforms.u_modelViewProj * float4(in.position, 1.0);
    out.color = in.color;
    return out;
}
