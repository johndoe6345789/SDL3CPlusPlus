#version 450

// GTA V terrain: four texture layers blended per vertex.
//
// A terrain shader (terrain_cb_w_4lyr and its variants) carries four
// diffuse layers -- sand, rock, dirt, grass -- and two weights: blue
// mixes layer 0 with 1 and layer 2 with 3, green mixes the two pairs.
// The weights are colour 1's; where the shader also has a lookup mask,
// the mask's instead, giving way to colour 1 as colour 0's alpha rises.
// The _cm shaders have no colour 1, and their mask alone decides.
//
// Lighting and haze match gta5_model.frag, so terrain and props meet
// without a seam.

layout(set = 2, binding = 0) uniform sampler2D layer0;
layout(set = 2, binding = 1) uniform sampler2D layer1;
layout(set = 2, binding = 2) uniform sampler2D layer2;
layout(set = 2, binding = 3) uniform sampler2D layer3;
layout(set = 2, binding = 4) uniform sampler2D lookupMask;

layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 u_lightDir;      // xyz = direction the light travels
    vec4 u_lightColor;    // rgb = colour * intensity, a = exposure
    vec4 u_ambient;       // rgb = ambient colour * intensity
    vec4 u_material;      // unused here
    vec4 u_fogColor;      // rgb = the sky step's horizon
    vec4 u_unused1;       // spotlight direction
    vec4 u_surface;       // rgb = tint, a = 1 when there is a lookup mask
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_cameraPos;
layout(location = 4) in vec4 v_shadowPos;
layout(location = 5) in vec4 v_blend;   // colour 1 b, g; colour 0 a
layout(location = 6) in vec2 v_uv1;     // where the mask is read

layout(location = 0) out vec4 o_color;

void main() {
    vec2 w = v_blend.xy;  // (blue, green)
    if (u_surface.a > 0.5) {
        w = mix(texture(lookupMask, v_uv1).bg, w, v_blend.z);
    }
    vec4 near = mix(texture(layer0, v_uv), texture(layer1, v_uv), w.x);
    vec4 far = mix(texture(layer2, v_uv), texture(layer3, v_uv), w.x);
    vec3 albedo = mix(near, far, w.y).rgb * u_surface.rgb;

    vec3 N = normalize(v_worldNormal);
    vec3 L = normalize(-u_lightDir.xyz);
    float wrap = max(dot(N, L) * 0.5 + 0.5, 0.0);
    vec3 lit = albedo * (u_lightColor.rgb * wrap + u_ambient.rgb);
    float exposure = (u_lightColor.a > 0.0) ? u_lightColor.a : 1.0;
    vec3 color = lit * exposure;

    float dist = length(v_worldPos - v_cameraPos);
    float fog = 1.0 - exp(-dist * 0.00035);
    o_color = vec4(mix(color, u_fogColor.rgb, fog), 1.0);
}
