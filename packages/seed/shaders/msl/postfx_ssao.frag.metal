#include <metal_stdlib>
using namespace metal;

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
};

struct SSAOParams {
    float4x4 u_projection;
    float4x4 u_inv_projection;
    float4   u_params;       // x=radius, y=bias, z=1/width, w=1/height
    float4   u_kernel[16];   // hemisphere sample directions
};

float3 reconstructViewPos(float2 uv, float depth, float4x4 invProj) {
    // UV to clip space [-1, 1]
    float4 clip = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip.y = -clip.y;  // Metal NDC convention
    float4 viewPos = invProj * clip;
    return viewPos.xyz / viewPos.w;
}

// Screen-space procedural noise for sample rotation
float2 noiseFromUV(float2 uv, float2 texelSize) {
    float2 screenPos = uv / texelSize;
    return fract(sin(float2(
        dot(screenPos, float2(12.9898, 78.233)),
        dot(screenPos, float2(39.346, 11.135))
    )) * 43758.5453);
}

fragment float4 main0(
    FragmentInput in [[stage_in]],
    texture2d<float> depthTex [[texture(0)]],
    sampler depthSampler [[sampler(0)]],
    constant SSAOParams& ssao [[buffer(0)]])
{
    float depth = depthTex.sample(depthSampler, in.uv).r;

    // Sky pixels: no occlusion
    if (depth >= 1.0) return float4(1.0);

    float radius = ssao.u_params.x;
    float bias   = ssao.u_params.y;
    float2 texelSize = ssao.u_params.zw;

    // Reconstruct view-space position and normal
    float3 fragPos = reconstructViewPos(in.uv, depth, ssao.u_inv_projection);
    float3 normal  = normalize(cross(dfdx(fragPos), dfdy(fragPos)));

    // Random rotation from procedural noise
    float2 noise = noiseFromUV(in.uv, texelSize);
    float angle = noise.x * 2.0 * M_PI_F;
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Build TBN from normal + random tangent
    float3 randomVec = float3(cosA, sinA, 0.0);
    float3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN     = float3x3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < 16; ++i) {
        // Orient sample in hemisphere around normal
        float3 sampleDir = TBN * ssao.u_kernel[i].xyz;
        float3 samplePos = fragPos + sampleDir * radius;

        // Project sample to screen space
        float4 offset = ssao.u_projection * float4(samplePos, 1.0);
        offset.xyz /= offset.w;
        float2 sampleUV = offset.xy * 0.5 + 0.5;
        sampleUV.y = 1.0 - sampleUV.y;

        // Sample depth at projected position
        float sampleDepth = depthTex.sample(depthSampler, sampleUV).r;
        float3 sampleViewPos = reconstructViewPos(sampleUV, sampleDepth, ssao.u_inv_projection);

        // Range check: only occlude if sample is within radius
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - sampleViewPos.z), 0.001));
        occlusion += (sampleViewPos.z >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    float ao = 1.0 - (occlusion / 16.0);
    return float4(ao, ao, ao, 1.0);
}
