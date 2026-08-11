#include <metal_stdlib>
using namespace metal;

struct VertexUniforms {
    float4x4 u_modelViewProj;
    float4x4 u_model;           // world-space model matrix
    float4   u_surfaceNormal;   // xyz = world-space surface normal
    float4   u_uvScale;         // xy = UV tile scale
    float4   u_cameraPos;       // xyz = camera world position
    float4x4 u_shadowVP;       // light view-projection matrix
};

struct VertexInput {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct VertexOutput {
    float4 position [[position]];
    float2 uv;
    float3 worldNormal;
    float3 worldPos;
    float3 cameraPos;
    float4 shadowPos;
};

vertex VertexOutput main0(
    VertexInput in [[stage_in]],
    constant VertexUniforms& uniforms [[buffer(0)]])
{
    VertexOutput out;
    out.position = uniforms.u_modelViewProj * float4(in.position, 1.0);
    out.uv = in.uv * uniforms.u_uvScale.xy;
    out.worldNormal = uniforms.u_surfaceNormal.xyz;
    float4 wp = uniforms.u_model * float4(in.position, 1.0);
    out.worldPos = wp.xyz;
    out.cameraPos = uniforms.u_cameraPos.xyz;
    out.shadowPos = uniforms.u_shadowVP * wp;
    return out;
}
