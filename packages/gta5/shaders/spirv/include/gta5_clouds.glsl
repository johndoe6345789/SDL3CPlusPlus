// Fair-weather cumulus for the procedural sky: value noise on a layer
// 1.5 km up, drifting with the wind, lit from the sun's side.

float CloudHash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float CloudNoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(CloudHash(i), CloudHash(i + vec2(1, 0)), u.x),
               mix(CloudHash(i + vec2(0, 1)), CloudHash(i + vec2(1, 1)), u.x),
               u.y);
}

float CloudFbm(vec2 p) {
    float sum = 0.0, amp = 0.5;
    for (int i = 0; i < 5; ++i) {
        sum += amp * CloudNoise(p);
        p = mat2(1.6, 1.2, -1.2, 1.6) * p;
        amp *= 0.5;
    }
    return sum;
}

// Mixes clouds over @p sky for a view along @p dir. @p light is the
// sun's colour and strength by day, the night's by night.
vec3 Clouds(vec3 sky, vec3 dir, vec3 eye, vec3 sun, float seconds,
            vec3 light, vec3 haze) {
    if (dir.y <= 0.01) return sky;
    vec2 at = (eye.xz + dir.xz / dir.y * 1500.0) / 2200.0;
    at += vec2(0.004, 0.0015) * seconds;  // ~9 m/s of wind
    float d = CloudFbm(at);
    float cover = smoothstep(0.52, 0.78, d);
    if (cover <= 0.0) return sky;
    // Brighter where the layer is thin and towards the sun, greyer in
    // the thick of it: the underside a cumulus shows from the street.
    float thick = smoothstep(0.55, 0.95, d);
    float toSun = pow(max(dot(dir, sun), 0.0), 4.0);
    vec3 lit = light * (0.55 + 0.35 * toSun) * (1.0 - 0.45 * thick);
    vec3 cloud = mix(haze, lit, 0.85);
    // Far off, the layer thins into the haze along the horizon.
    float far = smoothstep(0.02, 0.25, dir.y);
    return mix(sky, cloud, cover * far * 0.9);
}
