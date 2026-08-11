#include <metal_stdlib>
using namespace metal;

struct PBRUniforms {
    float4 u_lightDir;
    float4 u_lightColor;
    float4 u_ambient;
    float4 u_material;
    float4 u_flashPos;
    float4 u_flashDir;
    float4 u_flashColor;
};

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
    float2 lightmapUv;
    float3 worldNormal;
    float3 worldPos;
    float3 cameraPos;
};

fragment float4 main0(
    FragmentInput in [[stage_in]],
    texture2d<float> albedoTex [[texture(0)]],
    sampler albedoSampler [[sampler(0)]],
    texture2d<float> shadowMap [[texture(1)]],
    sampler shadowSampler [[sampler(1)]],
    texture2d<float> lightmapTex [[texture(2)]],
    sampler lightmapSampler [[sampler(2)]],
    texture2d<float> portalTex [[texture(3)]],
    sampler portalSampler [[sampler(3)]],
    constant PBRUniforms& pbr [[buffer(0)]])
{
    (void)shadowMap;
    (void)shadowSampler;
    (void)in.worldNormal;

    float3 albedo = albedoTex.sample(albedoSampler, in.uv).rgb;
    float3 lightmap = lightmapTex.sample(lightmapSampler, in.lightmapUv).rgb;
    float overbright = (pbr.u_material.z > 0.0) ? pbr.u_material.z : 2.0;
    float3 ambient = pbr.u_ambient.rgb * albedo;
    float exposure = (pbr.u_lightColor.a > 0.0) ? pbr.u_lightColor.a : 1.0;

    if (pbr.u_material.w > 0.5) {
        float time = pbr.u_material.y;
        float3 viewDir = normalize(in.cameraPos - in.worldPos);
        float3 n = normalize(in.worldNormal);
        float3 reflected = reflect(-viewDir, n);
        float fresnel = pow(1.0 - saturate(dot(viewDir, n)), 3.0);

        float2 reflectUv = reflected.xz * 0.32 + float2(0.5, 0.5);
        reflectUv += float2(sin(time * 1.7 + in.worldPos.y * 0.35),
                            cos(time * 1.3 + in.worldPos.x * 0.28)) * 0.035;

        float3 portalBase = albedoTex.sample(albedoSampler, reflectUv).rgb;
        float2 portalUv = fract(in.uv);
        portalUv.y = 1.0 - portalUv.y;
        float2 centered = portalUv - float2(0.5, 0.5);
        float radial = length(centered);
        float centerMask = 1.0 - smoothstep(0.34, 0.49, radial);
        float3 destination = portalTex.sample(portalSampler, portalUv).rgb;
        float pulse = 0.5 + 0.5 * sin(time * 3.0 + length(in.worldPos.xz) * 0.45);
        float3 glow = float3(0.18, 0.42, 0.95) * (0.35 + 0.35 * pulse);
        float3 reflectedColor = mix(portalBase, glow, 0.18 + 0.32 * fresnel);
        float3 color = mix(reflectedColor, destination, centerMask * 0.88);
        color += glow * (1.0 - centerMask) * 0.45;
        float alpha = mix(0.18 + 0.24 * fresnel, 0.92, centerMask);
        return float4((color * 1.08 + ambient * 0.08) * exposure, alpha);
    }

    return float4((albedo * lightmap * overbright + ambient) * exposure, 1.0);
}
