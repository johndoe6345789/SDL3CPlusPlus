#include <metal_stdlib>
using namespace metal;

// PBR Cook-Torrance + PCF Shadow Mapping + Hemisphere Ambient
// Outputs linear HDR — tonemapping handled by postfx composite

struct PBRUniforms {
    float4 u_lightDir;      // xyz = direction (toward scene), w = unused
    float4 u_lightColor;    // rgb = light color * intensity, a = exposure
    float4 u_ambient;       // rgb = ambient color * intensity, a = unused
    float4 u_material;      // x = roughness, y = metallic, zw = unused
};

struct FragmentInput {
    float4 position [[position]];
    float2 uv;
    float3 worldNormal;
    float3 worldPos;
    float3 cameraPos;
    float4 shadowPos;
};

float D_GGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (M_PI_F * d * d);
}

float G_SchlickGGX(float NdotX, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

float3 F_Schlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ComputeShadowPCF(float4 shadowPos, float NdotL,
                       texture2d<float> shadowMap, sampler shadowSampler) {
    float3 ndc = shadowPos.xyz / shadowPos.w;
    float2 shadowUV = ndc.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0;

    float currentDepth = ndc.z;

    // Angle-based bias: steeper angles need more bias to avoid acne
    float bias = max(0.005 * (1.0 - NdotL), 0.001);

    // 5x5 PCF kernel
    float texelSize = 1.0 / float(shadowMap.get_width());
    float shadow = 0.0;
    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            float2 offset = float2(float(x), float(y)) * texelSize;
            float sampleDepth = shadowMap.sample(shadowSampler, shadowUV + offset).r;
            shadow += (currentDepth - bias > sampleDepth) ? 0.0 : 1.0;
        }
    }
    shadow /= 25.0;

    // Minimum shadow (ambient light still reaches shadowed areas)
    return mix(0.12, 1.0, shadow);
}

fragment float4 main0(
    FragmentInput in [[stage_in]],
    texture2d<float> albedoTex [[texture(0)]],
    sampler albedoSampler [[sampler(0)]],
    texture2d<float> shadowMap [[texture(1)]],
    sampler shadowSampler [[sampler(1)]],
    constant PBRUniforms& pbr [[buffer(0)]])
{
    float3 albedo    = albedoTex.sample(albedoSampler, in.uv).rgb;
    float  roughness = pbr.u_material.x;
    float  metallic  = pbr.u_material.y;
    float  exposure  = pbr.u_lightColor.a;

    float3 N = normalize(in.worldNormal);
    float3 L = normalize(-pbr.u_lightDir.xyz);
    float3 V = normalize(in.cameraPos - in.worldPos);
    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float3 F0 = mix(float3(0.04), albedo, metallic);

    float  D = D_GGX(NdotH, roughness);
    float  G = G_Smith(NdotV, NdotL, roughness);
    float3 F = F_Schlick(HdotV, F0);

    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);

    float3 kD = (float3(1.0) - F) * (1.0 - metallic);
    float3 diffuse = kD * albedo / M_PI_F;

    // PCF soft shadow with angle-based bias
    float shadow = ComputeShadowPCF(in.shadowPos, NdotL, shadowMap, shadowSampler);

    // Outgoing radiance (shadowed)
    float3 Lo = (diffuse + specular) * pbr.u_lightColor.rgb * NdotL * shadow;

    // Hemisphere ambient: sky tint above, warm ground tint below
    float3 skyColor    = pbr.u_ambient.rgb;
    float3 groundColor = pbr.u_ambient.rgb * float3(0.6, 0.5, 0.4) * 0.5;
    float  hemisphere  = N.y * 0.5 + 0.5; // 0 = down, 1 = up
    float3 ambient     = mix(groundColor, skyColor, hemisphere) * albedo;

    // Output linear HDR — tonemapping in composite pass
    float3 color = (Lo + ambient) * exposure;
    return float4(color, 1.0);
}
