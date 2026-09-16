#version 450
#extension GL_GOOGLE_include_directive : require

// Streamed GTA V map geometry: textured, sun and sky lit, hazed.
//
// Not the seed textured.frag: its fog is 99.95% opaque by 128 m, which
// turned a city into black silhouettes, and it ray-marches a volumetric
// beam per fragment, far too dear across a skyline.

layout(set = 2, binding = 0) uniform sampler2D albedoTex;
layout(set = 2, binding = 1) uniform sampler2DShadow shadowMap;
layout(set = 2, binding = 2) uniform sampler2D waterMap;

// The engine's 112-byte FragmentUniformData. Its spotlight slots carry
// the sky's horizon colour and the submesh's surface: GTA's paint
// textures are a few white pixels coloured per vehicle, and a leaf is a
// rectangle whose shape is all in its alpha.
layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 u_lightDir;      // xyz = direction the light travels
    vec4 u_lightColor;    // rgb = colour * intensity, a = exposure
    vec4 u_ambient;       // rgb = ambient colour * intensity
    vec4 u_material;      // z = how deep the camera is under water
    vec4 u_fogColor;      // rgb = the sky step's horizon
    vec4 u_unused1;       // spotlight direction
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
    // Discard rather than blend: a cutout's alpha says "not here", and
    // discarding needs no sorting where blending a city would.
    if (u_surface.a > 0.0 && texel.a < u_surface.a) discard;

    vec3 albedo = SrgbToLinear(texel.rgb) * SrgbToLinear(u_surface.rgb);
    vec3 N = normalize(v_worldNormal);
    // Foliage draws with culling off: a card's far side is lit as its
    // own front, or every leaf turned dark as the view went round it.
    if (!gl_FrontFacing) N = -N;

    // Left linear: the composite tone maps and encodes, once.
    vec3 color = Atmosphere(Shade(albedo, N), Exposure());
    // Alpha matters only to the blended pipeline: decals and glass.
    o_color = vec4(color, texel.a);
}
