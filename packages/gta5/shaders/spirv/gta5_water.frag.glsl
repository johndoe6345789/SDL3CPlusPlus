#version 450

// Water: swell normals from a few travelling sines, the sky -- or the
// reflection pass, where there is one -- mirrored in it by Fresnel, the
// sun's glint, and the city's haze with distance. The sky and the
// reflection are already in display range; the water's own colour is
// lit like the city and scaled by its exposure.

layout(set = 2, binding = 0) uniform sampler2D reflectionTex;

layout(set = 3, binding = 0) uniform WaterUniforms {
    vec4 u_lightDir;    // xyz = direction the light travels
    vec4 u_lightColor;  // rgb = colour * intensity
    vec4 u_ambient;
    vec4 u_horizon;     // the clock's sky
    vec4 u_zenith;
    vec4 u_cameraPos;
    vec4 u_params;      // x = seconds, y = exposure, z = reflection on
    vec4 u_screen;      // xy = 1 / render target size
};

layout(location = 0) in vec3 v_worldPos;

layout(location = 0) out vec4 o_color;

vec2 Swell(vec2 p, vec2 k, float speed, float height, float t) {
    return k * height * cos(dot(p, k) + t * speed);
}

vec3 WaveNormal(vec2 p, float t) {
    vec2 g = Swell(p, vec2(0.12, 0.05), 1.1, 0.6, t) +
             Swell(p, vec2(-0.07, 0.15), 1.4, 0.45, t) +
             Swell(p, vec2(0.31, -0.22), 2.3, 0.2, t) +
             Swell(p, vec2(-0.53, -0.41), 3.1, 0.1, t);
    return normalize(vec3(-g.x, 1.0, -g.y));
}

void main() {
    vec3 N = WaveNormal(v_worldPos.xz, u_params.x);
    vec3 toEye = u_cameraPos.xyz - v_worldPos;
    vec3 V = normalize(toEye);
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(N, V), 0.0), 5.0);
    vec3 R = reflect(-V, N);
    vec3 mirrored = mix(u_horizon.rgb, u_zenith.rgb,
                        pow(clamp(R.y, 0.0, 1.0), 0.35));
    // The mirror is sea level: lakes up the hills keep the sky.
    if (u_params.z > 0.5 && abs(v_worldPos.y) < 2.0) {
        vec2 uv = gl_FragCoord.xy * u_screen.xy + N.xz * 0.04;
        vec4 seen = texture(reflectionTex, uv);
        mirrored = mix(mirrored, seen.rgb, seen.a);
    }
    vec3 L = normalize(-u_lightDir.xyz);
    float exposure = u_params.y;
    vec3 deep = vec3(0.02, 0.07, 0.09) *
                (u_ambient.rgb + u_lightColor.rgb * max(L.y, 0.0)) * exposure;
    float glint = pow(max(dot(R, L), 0.0), 500.0) * 6.0;
    vec3 color = mix(deep, mirrored, fresnel) +
                 u_lightColor.rgb * glint * exposure;
    float fog = 1.0 - exp(-length(toEye) * 0.00035);
    color = mix(color, u_horizon.rgb, fog);
    o_color = vec4(color, mix(0.82, 0.97, fresnel));
}
