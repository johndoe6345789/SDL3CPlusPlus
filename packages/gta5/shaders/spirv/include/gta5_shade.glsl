// How the sun and sky light a surface. Needs gta5_sun_shadow.glsl and
// the PBRUniforms block (u_lightDir, u_lightColor, u_ambient).

#include "gta5_srgb.glsl"

float Exposure() {
    return u_lightColor.a > 0.0 ? u_lightColor.a : 1.0;
}

// Lambert for the sun, and the sky for the rest: full from above, less
// from the side, least from below, where only the ground bounces light.
// A wrapped sun, which this replaced, lit a face turned away at half
// strength and left sun and shade barely apart.
vec3 Shade(vec3 albedo, vec3 N) {
    vec3 L = normalize(-u_lightDir.xyz);
    float sun = max(dot(N, L), 0.0) * SunShadow();
    float sky = mix(0.55, 1.0, N.y * 0.5 + 0.5);
    vec3 lit = albedo * (u_lightColor.rgb * sun + u_ambient.rgb * sky);
    return lit * Exposure();
}
