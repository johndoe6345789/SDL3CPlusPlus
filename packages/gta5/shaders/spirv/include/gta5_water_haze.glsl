// Water and haze, laid over what the sun and sky lit.
//
// Needs: sampler2D waterMap; in vec3 v_worldPos, v_cameraPos; the
// PBRUniforms block (u_material, u_fogColor).

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
    if (u_material.z > 0.0) {
        color = mix(color, murk, 1.0 - exp(-dist * 0.07));
    }
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

// Haze tuned for kilometres rather than a room. Its colour is the sky
// step's horizon, so the two cannot drift apart and leave a band.
vec3 Atmosphere(vec3 color, float exposure) {
    float dist = length(v_worldPos - v_cameraPos);
    vec3 light = (u_ambient.rgb + u_lightColor.rgb * 0.6) * exposure;
    color = Submerge(color, dist, light);
    return mix(color, u_fogColor.rgb, Haze(v_cameraPos, v_worldPos));
}
