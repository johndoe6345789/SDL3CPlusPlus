#include <metal_stdlib>
using namespace metal;

// Ground fragment: slope and altitude pick grass, dirt, rock and pale high
// ground. Normals come from screen-space derivatives of the position, so the
// terrain keeps the position + uv vertex format.

struct PBRUniforms {
    float4 u_lightDir;      // xyz = direction (toward scene)
    float4 u_lightColor;    // rgb = light colour * intensity, a = exposure
    float4 u_ambient;       // rgb = ambient colour * intensity
    float4 u_material;
    float4 u_flashPos;
    float4 u_flashDir;
    float4 u_flashColor;
};

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
    float3 worldNormal;
    float3 worldPos;
    float3 cameraPos;
    float4 shadowPos;
};

constant float3 kDirt = float3(0.42, 0.33, 0.22);
constant float3 kRock = float3(0.46, 0.44, 0.41);
constant float3 kPale = float3(0.58, 0.55, 0.50);
constant float3 kFog = float3(0.55, 0.66, 0.80);

fragment float4 main0(
    FragmentInput in [[stage_in]],
    texture2d<float> groundTex [[texture(0)]],
    sampler groundSampler [[sampler(0)]],
    constant PBRUniforms& pbr [[buffer(0)]])
{
    float3 N = normalize(cross(dfdx(in.worldPos), dfdy(in.worldPos)));
    if (N.y < 0.0) N = -N;

    float3 detail = groundTex.sample(groundSampler, in.uv).rgb;
    float slope = 1.0 - N.y;
    float3 albedo = mix(detail, kDirt * (0.8 + 0.4 * detail.r), 0.35);
    albedo = mix(albedo, kRock * (0.85 + 0.3 * detail.g),
                 smoothstep(0.2, 0.4, slope));
    albedo = mix(albedo, kPale,
                 smoothstep(230.0, 330.0, in.worldPos.y) * 0.7);

    float3 L = normalize(-pbr.u_lightDir.xyz);
    float NdotL = max(dot(N, L), 0.0);
    float3 lit = albedo / M_PI_F * pbr.u_lightColor.rgb * NdotL;

    float hemisphere = N.y * 0.5 + 0.5;
    float3 groundAmbient = pbr.u_ambient.rgb * float3(0.6, 0.5, 0.4) * 0.5;
    float3 ambient = mix(groundAmbient, pbr.u_ambient.rgb, hemisphere) * albedo;
    float3 color = (lit + ambient) * pbr.u_lightColor.a;

    float dist = length(in.worldPos - in.cameraPos);
    float fog = 1.0 - exp(-dist * 0.0009);
    return float4(mix(color, kFog, fog), 1.0);
}
