#version 450

// Ground fragment: slope and altitude pick grass, dirt, rock and pale high
// ground. Normals come from screen-space derivatives of the position, so the
// terrain keeps the position + uv vertex format.

// SDL3 GPU: set=3 for fragment uniform buffers
layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 u_lightDir;      // xyz = direction (toward scene)
    vec4 u_lightColor;    // rgb = light colour * intensity, a = exposure
    vec4 u_ambient;       // rgb = ambient colour * intensity
    vec4 u_material;
    vec4 u_flashPos;
    vec4 u_flashDir;
    vec4 u_flashColor;
};

// SDL3 GPU: set=2 for fragment combined image samplers
layout(set = 2, binding = 0) uniform sampler2D groundTex;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_cameraPos;
layout(location = 4) in vec4 v_shadowPos;

layout(location = 0) out vec4 o_color;

const float PI = 3.14159265359;
const vec3 kDirt = vec3(0.42, 0.33, 0.22);
const vec3 kRock = vec3(0.46, 0.44, 0.41);
const vec3 kPale = vec3(0.58, 0.55, 0.50);
const vec3 kFog = vec3(0.55, 0.66, 0.80);

void main() {
    vec3 N = normalize(cross(dFdx(v_worldPos), dFdy(v_worldPos)));
    if (N.y < 0.0) N = -N;

    vec3 detail = texture(groundTex, v_uv).rgb;
    float slope = 1.0 - N.y;
    vec3 albedo = mix(detail, kDirt * (0.8 + 0.4 * detail.r), 0.35);
    albedo = mix(albedo, kRock * (0.85 + 0.3 * detail.g),
                 smoothstep(0.2, 0.4, slope));
    albedo = mix(albedo, kPale,
                 smoothstep(230.0, 330.0, v_worldPos.y) * 0.7);

    vec3 L = normalize(-u_lightDir.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec3 lit = albedo / PI * u_lightColor.rgb * NdotL;

    float hemisphere = N.y * 0.5 + 0.5;
    vec3 groundAmbient = u_ambient.rgb * vec3(0.6, 0.5, 0.4) * 0.5;
    vec3 ambient = mix(groundAmbient, u_ambient.rgb, hemisphere) * albedo;
    vec3 color = (lit + ambient) * u_lightColor.a;

    float dist = length(v_worldPos - v_cameraPos);
    float fog = 1.0 - exp(-dist * 0.0009);
    o_color = vec4(mix(color, kFog, fog), 1.0);
}
