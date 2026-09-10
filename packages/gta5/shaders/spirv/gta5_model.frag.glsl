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

layout(set = 3, binding = 0) uniform PBRUniforms {
    vec4 u_lightDir;      // xyz = direction the light travels
    vec4 u_lightColor;    // rgb = colour * intensity, a = exposure
    vec4 u_ambient;       // rgb = ambient colour * intensity
    vec4 u_material;      // x = roughness, y = metallic (unused here)
    vec4 u_flashPos;
    vec4 u_flashDir;
    vec4 u_flashColor;
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_cameraPos;
layout(location = 4) in vec4 v_shadowPos;

layout(location = 0) out vec4 o_color;

void main() {
    vec3 albedo = texture(albedoTex, v_uv).rgb;
    vec3 N = normalize(v_worldNormal);
    vec3 L = normalize(-u_lightDir.xyz);

    // Wrapped diffuse: keeps faces turned away from the sun readable
    // instead of pure black, which matters when half a tower faces away.
    float wrap = max(dot(N, L) * 0.5 + 0.5, 0.0);

    vec3 lit = albedo * (u_lightColor.rgb * wrap + u_ambient.rgb);
    float exposure = (u_lightColor.a > 0.0) ? u_lightColor.a : 1.0;

    // Reinhard tone map. The seed shader leaves this to the postfx
    // composite, which this package does not run, so without it the road
    // right under the camera clips to pure white.
    vec3 color = lit * exposure;
    color = color / (color + vec3(1.0));

    // Haze tuned for kilometres rather than a room: still legible out at
    // the far edge of the streamed tiles.
    float dist = length(v_worldPos - v_cameraPos);
    float fog = 1.0 - exp(-dist * 0.00035);
    color = mix(color, vec3(0.55, 0.60, 0.70), fog);

    o_color = vec4(color, 1.0);
}
