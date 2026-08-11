#version 450

// Quake 3 BSP fragment shader (minimal).
// Q3 bakes radiosity into a lightmap atlas at compile time, so runtime lighting
// is just: albedo × lightmap × overbright. The classic Q3 "overbright bits = 1"
// doubles lightmap intensity — modern screens are brighter, so we expose it as
// part of the existing FragmentUniformData.material slot (z) with a 2.0 default.
//
// Fragment sampler bindings come from workflow_draw_map_step.cpp BSP path:
//   set=2 binding=0  albedo    (per-surface diffuse from extracted pk3 textures)
//   set=2 binding=1  shadowMap (shadow map — ignored for BSP, lightmap is canonical)
//   set=2 binding=2  lightmap  (shared atlas built by bsp.lightmap_atlas)

layout(set = 2, binding = 0) uniform sampler2D albedoTex;
layout(set = 2, binding = 1) uniform sampler2D shadowMap;
layout(set = 2, binding = 2) uniform sampler2D lightmapTex;

layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 u_lightDir;
    vec4 u_lightColor;    // a = exposure
    vec4 u_ambient;
    vec4 u_material;      // z = lightmap overbright multiplier (defaults via C++)
    vec4 u_flashPos;
    vec4 u_flashDir;
    vec4 u_flashColor;
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec2 v_lmuv;
layout(location = 2) in vec3 v_worldNormal;
layout(location = 3) in vec3 v_worldPos;
layout(location = 4) in vec3 v_cameraPos;

layout(location = 0) out vec4 o_color;

void main() {
    vec3 albedo   = texture(albedoTex, v_uv).rgb;
    vec3 lightmap = texture(lightmapTex, v_lmuv).rgb;

    // Q3 overbright. material.z falls back to 2.0 (engine pushes 0 today, so
    // fall through to a sane default rather than rendering a black map).
    float overbright = (u_material.z > 0.0) ? u_material.z : 2.0;

    vec3 ambient = u_ambient.rgb * albedo;
    vec3 lit = albedo * lightmap * overbright + ambient;

    float exposure = (u_lightColor.a > 0.0) ? u_lightColor.a : 1.0;
    o_color = vec4(lit * exposure, 1.0);
}
