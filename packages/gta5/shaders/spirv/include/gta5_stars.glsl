// Stars: a sparse hash of the view direction, faded in with the night,
// which the clock puts in the horizon's alpha (@p night).
vec3 Stars(vec3 dir, float night) {
    if (night <= 0.0 || dir.y <= 0.0) return vec3(0.0);
    vec3 cell = floor(dir * 320.0);
    float h = fract(sin(dot(cell, vec3(12.9898, 78.233, 37.719))) *
                    43758.5453);
    float star = step(0.9972, h) * night * smoothstep(0.0, 0.2, dir.y);
    return vec3(0.9, 0.93, 1.0) * star * (0.5 + 0.5 * fract(h * 97.0));
}
