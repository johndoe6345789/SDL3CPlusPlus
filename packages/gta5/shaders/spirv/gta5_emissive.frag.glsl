#version 450
#extension GL_GOOGLE_include_directive : require

// GTA V's emissive materials -- lit windows, signs, neon -- on streamed
// geometry: gta5_model.frag by day, plus the texture's own light after
// dark, faded in with gta5.time.night.

layout(set = 2, binding = 0) uniform sampler2D albedoTex;
layout(set = 2, binding = 1) uniform sampler2DShadow shadowMap;
layout(set = 2, binding = 2) uniform sampler2D waterMap;

// gta5_model.frag's uniforms, with one spotlight slot for the night.
layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 u_lightDir;      // xyz = direction the light travels
    vec4 u_lightColor;    // rgb = colour * intensity, a = exposure
    vec4 u_ambient;       // rgb = ambient colour * intensity
    vec4 u_material;      // z = how deep the camera is under water
    vec4 u_fogColor;      // rgb = the sky step's horizon
    vec4 u_night;         // x: 0 by day, 1 at night (gta5.time.night)
    vec4 u_surface;       // rgb = tint, a = alpha discard, 0 = off
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_cameraPos;
layout(location = 4) in vec4 v_shadowPos;

layout(location = 0) out vec4 o_color;

#include "include/gta5_sun_shadow.glsl"
#include "include/gta5_shade.glsl"
#include "include/gta5_water_haze.glsl"

void main() {
    vec4 texel = texture(albedoTex, v_uv);
    if (u_surface.a > 0.0 && texel.a < u_surface.a) discard;

    vec3 albedo = SrgbToLinear(texel.rgb) * SrgbToLinear(u_surface.rgb);
    vec3 N = normalize(v_worldNormal);
    if (!gl_FrontFacing) N = -N;

    vec3 color = Shade(albedo, N);
    // Its own light: not scaled by exposure, which is for what the sun
    // and moon light, and not shaded by them either.
    color += albedo * clamp(u_night.x, 0.0, 1.0) * 1.2;
    color = Atmosphere(color, Exposure());
    o_color = vec4(color, texel.a);
}
