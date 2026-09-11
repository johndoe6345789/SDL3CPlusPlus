#version 450

// Procedural sky for the streamed city.
//
// No cube map: GTA V's own sky is a time-of-day system fed by
// timecycle .xml that is not in the extract, so this builds one from
// the same sun direction and haze colour the model shader already
// uses. That keeps the horizon and the distance fog the same colour,
// which is the join you notice if it is wrong.

layout(set = 3, binding = 0) uniform SkyUniforms {
    mat4 u_invViewProj;   // clip space back to a world direction
    vec4 u_cameraPos;     // xyz = eye, w unused
    vec4 u_sunDir;        // xyz = direction the light travels
    vec4 u_horizon;       // rgb = haze colour, matches the model fog; a = stars
    vec4 u_zenith;        // rgb = sky overhead, a = sun size
};

layout(location = 0) in vec2 v_ndc;
layout(location = 0) out vec4 o_color;

void main() {
    // Unproject the far plane, then subtract the eye: the result is the
    // direction this pixel looks in, which is all a sky needs.
    vec4 far = u_invViewProj * vec4(v_ndc, 1.0, 1.0);
    vec3 dir = normalize(far.xyz / far.w - u_cameraPos.xyz);
    // Under water (the clock puts the flag in the sun's w): murk, in the
    // day's own light.
    if (u_sunDir.w > 0.0) {
        o_color = vec4(u_horizon.rgb * vec3(0.15, 0.45, 0.5), 1.0);
        return;
    }
    vec3 sun = normalize(-u_sunDir.xyz);

    // Sky darkens with height off the horizon. The 0.35 power keeps the
    // gradient in the lower part of the view, where a linear ramp puts
    // it all overhead and leaves the horizon flat.
    float height = clamp(dir.y, 0.0, 1.0);
    vec3 color = mix(u_horizon.rgb, u_zenith.rgb, pow(height, 0.35));

    // Glow around the sun, then the disc. Both widen towards the
    // horizon by the same height term, which is what makes a low sun
    // read as low rather than as a lamp stuck on a wall.
    float toSun = max(dot(dir, sun), 0.0);
    float haze = pow(toSun, 8.0) * (1.0 - height * 0.7);
    color += u_horizon.rgb * haze * 0.6;
    color += vec3(1.0, 0.92, 0.78) * pow(toSun, 2200.0) * u_zenith.a;

    // Stars: a sparse hash of the view direction, faded in with the
    // night, which the clock puts in the horizon's alpha.
    if (u_horizon.a > 0.0 && dir.y > 0.0) {
        vec3 cell = floor(dir * 320.0);
        float h = fract(sin(dot(cell, vec3(12.9898, 78.233, 37.719))) *
                        43758.5453);
        float star = step(0.9972, h) * u_horizon.a * smoothstep(0.0, 0.2, dir.y);
        color += vec3(0.9, 0.93, 1.0) * star * (0.5 + 0.5 * fract(h * 97.0));
    }
    // Below the horizon is ground haze, not sky: the streamed tiles run
    // out long before the view does, and an unfilled lower half reads as
    // a hole rather than as distance.
    float below = clamp(-dir.y * 6.0, 0.0, 1.0);
    color = mix(color, u_horizon.rgb * 0.8, below);

    o_color = vec4(color, 1.0);
}
