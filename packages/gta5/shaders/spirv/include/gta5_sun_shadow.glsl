// The sun's shadow map (gta5.shadow.draw), sampled nine times for a soft
// edge and faded out towards the map's rim, where it ends.
//
// Needs: sampler2DShadow shadowMap; in vec4 v_shadowPos.
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
