#version 450

// Streamed GTA V map geometry.
//
// Deliberately not the seed textured.frag. That one is a room-scale demo
// shader: its fog is hardcoded at 1 - exp(-dist * 0.06), which is 99.95%
// opaque by 128 m, so on a city every building past the near kerb
// flattened to the fog colour and read as a black silhouette. It also
// ray-marches a 48-step volumetric light beam per fragment, which is far
// too expensive across a whole skyline.
//
// This is a plain textured Lambert with a wrap term and city-scale haze.

layout(set = 2, binding = 0) uniform sampler2D albedoTex;
layout(set = 2, binding = 1) uniform sampler2DShadow shadowMap;
layout(set = 2, binding = 2) uniform sampler2D waterMap;

// Same 112 bytes the engine pushes as FragmentUniformData. The last
// three vec4s are a spotlight this shader has no use for, so the draw
// reuses two of them: the first for the sky's horizon colour, and the
// last for the submesh's own surface -- GTA V paint
// textures are a few white pixels with the colour supplied per vehicle,
// and foliage is a rectangle whose shape is entirely in its alpha.
layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 u_lightDir;      // xyz = direction the light travels
    vec4 u_lightColor;    // rgb = colour * intensity, a = exposure
    vec4 u_ambient;       // rgb = ambient colour * intensity
    vec4 u_material;      // x = roughness, y = metallic (unused here)
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

// The sun's shadow map (gta5.shadow.draw), sampled nine times for a soft
// edge and faded out towards the map's rim, where it ends.
float SunShadow() {
    vec3 p = v_shadowPos.xyz / v_shadowPos.w;
    float edge = max(abs(p.x), abs(p.y));
    if (edge >= 1.0 || p.z <= 0.0 || p.z >= 1.0) return 1.0;
    vec2 uv = vec2(p.x * 0.5 + 0.5, 0.5 - p.y * 0.5);
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float lit = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            lit += texture(shadowMap,
                           vec3(uv + vec2(x, y) * texel, p.z - 0.00005));
        }
    }
    return mix(lit / 9.0, 1.0, smoothstep(0.85, 1.0, edge));
}

// Water fills what lies under its surface (gta5.water.map: the heights of
// water.xml over the map; GTA's y is engine -z), the deeper the more of
// its own colour. u_material.z is how deep the camera is: under, all of
// it goes murky.
vec3 Submerge(vec3 color, float dist, vec3 light) {
    vec2 uv = vec2((v_worldPos.x + 4140.0) / 9000.0,
                   (8400.0 + v_worldPos.z) / 13500.0);
    float depth = texture(waterMap, uv).r - v_worldPos.y;
    vec3 murk = vec3(0.02, 0.09, 0.10) * light;
    if (depth > 0.0) {
        color = mix(color * vec3(0.75, 0.9, 0.95), murk,
                    1.0 - exp(-depth * 0.12));
    }
    if (u_material.z > 0.0) color = mix(color, murk, 1.0 - exp(-dist * 0.07));
    return color;
}

// Haze is thick low down and thins with height (e-folding 600 m), taken
// along the ray: from a hilltop the valleys show through half of it,
// and a view across at that height through a quarter.
float Haze(vec3 eye, vec3 p) {
    float k = 1.0 / 600.0;
    float a = max(eye.y, 0.0), b = max(p.y, 0.0);
    float mean = exp(-a * k);
    if (abs(b - a) > 1.0) mean = (mean - exp(-b * k)) / ((b - a) * k);
    return 1.0 - exp(-length(p - eye) * 0.0004 * mean);
}

void main() {
    vec4 texel = texture(albedoTex, v_uv);
    // Discard rather than blend: a cutout's alpha says "not here", and
    // discarding needs no sorting where blending across a streamed city
    // would.
    if (u_surface.a > 0.0 && texel.a < u_surface.a) discard;

    vec3 albedo = texel.rgb * u_surface.rgb;
    vec3 N = normalize(v_worldNormal);
    // Foliage draws with culling off: a card's far side is lit as its
    // own front, or every leaf turned dark as the view went round it.
    if (!gl_FrontFacing) N = -N;
    vec3 L = normalize(-u_lightDir.xyz);

    // Wrapped diffuse: keeps faces turned away from the sun readable
    // instead of pure black, which matters when half a tower faces away.
    float wrap = max(dot(N, L) * 0.5 + 0.5, 0.0);

    vec3 lit = albedo * (u_lightColor.rgb * wrap * SunShadow() +
                         u_ambient.rgb);
    float exposure = (u_lightColor.a > 0.0) ? u_lightColor.a : 1.0;

    // Left linear and untonemapped: the package's composite does ACES
    // and gamma on the way to the swapchain, and tone mapping twice
    // washes the whole city out.
    vec3 color = lit * exposure;

    // Haze tuned for kilometres rather than a room: still legible out at
    // the far edge of the streamed tiles. The colour comes from the sky
    // step, so the two cannot drift apart and leave a band on the
    // horizon.
    float dist = length(v_worldPos - v_cameraPos);
    color = Submerge(color, dist,
                     (u_ambient.rgb + u_lightColor.rgb * 0.6) * exposure);
    float fog = Haze(v_cameraPos, v_worldPos);
    color = mix(color, u_fogColor.rgb, fog);

    // Alpha matters only to the blended pipeline: decals and glass.
    o_color = vec4(color, texel.a);
}
