#version 450

// FS2024's own tree imposters: a BC7 sampler2DArray, one layer per
// variation, alpha-cut to the canopy's own silhouette. Lit and fogged
// the same way the ground is, so a forest sits in the same light.

layout(set = 2, binding = 0) uniform sampler2DArray species_map;

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec4 u_sunDir;      // direction the light travels
    vec4 u_sunColour;   // rgb = colour * intensity, a = exposure
    vec4 u_ambient;     // rgb = sky colour * intensity
    vec4 u_fog;         // rgb = linear horizon colour, a = density per m
    vec4 u_runway;      // unused here
    vec4 u_runwayAxis;
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_cameraPos;
layout(location = 4) in float v_layer;
layout(location = 0) out vec4 o_color;

void main() {
    vec4 tex = texture(species_map, vec3(v_uv, v_layer));
    if (tex.a < 0.5) discard;
    vec3 albedo = pow(tex.rgb, vec3(2.2));

    // Both faces of a crossed quad get the sun: a real tree scatters
    // light through its canopy far more than a flat wall does.
    vec3 n = normalize(v_normal);
    float sun = 0.35 + 0.65 * abs(dot(n, normalize(u_sunDir.xyz)));
    float sky = 0.7;
    vec3 lit = albedo * (u_sunColour.rgb * sun + u_ambient.rgb * sky);
    lit *= u_sunColour.a > 0.0 ? u_sunColour.a : 1.0;

    float distance = length(v_worldPos - v_cameraPos);
    float fog = 1.0 - exp(-distance * u_fog.a);
    o_color = vec4(mix(lit, u_fog.rgb, fog), 1.0);
}
