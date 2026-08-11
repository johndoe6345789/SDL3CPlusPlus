#include <metal_stdlib>
using namespace metal;

struct ShadowUniforms {
    float4x4 u_lightVP;
    float4x4 u_model;
};

struct VertexInput {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(1)]];  // present in vertex buffer but unused
};

vertex float4 main0(
    VertexInput in [[stage_in]],
    constant ShadowUniforms& uniforms [[buffer(0)]])
{
    return uniforms.u_lightVP * uniforms.u_model * float4(in.position, 1.0);
}
