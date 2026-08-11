#include <metal_stdlib>
using namespace metal;

struct VertexUniforms {
    float4x4 u_modelViewProj;
    float4x4 u_model;
    float4   u_surfaceNormal;
    float4   u_uvScale;
    float4   u_cameraPos;
    float4x4 u_shadowVP;
};

struct VertexInput {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
    float2 lightmapUv [[attribute(2)]];
    float3 normal [[attribute(3)]];
};

struct VertexOutput {
    float4 position [[position]];
    float2 uv;
    float2 lightmapUv;
    float3 worldNormal;
    float3 worldPos;
    float3 cameraPos;
};

vertex VertexOutput main0(
    VertexInput in [[stage_in]],
    constant VertexUniforms& uniforms [[buffer(0)]])
{
    VertexOutput out;
    out.position = uniforms.u_modelViewProj * float4(in.position, 1.0);
    out.uv = in.uv * uniforms.u_uvScale.xy;
    out.lightmapUv = in.lightmapUv;

    float4 worldPos = uniforms.u_model * float4(in.position, 1.0);
    out.worldPos = worldPos.xyz;
    out.worldNormal = (uniforms.u_model * float4(in.normal, 0.0)).xyz;
    out.cameraPos = uniforms.u_cameraPos.xyz;
    return out;
}
