// A lit surface's normal and specular maps, where it has them.
// Needs: sampler2D bumpTex, specTex; in vec2 v_uv; in vec3 v_worldPos,
// v_cameraPos; the PBRUniforms block (u_material x: has a normal map,
// y: has a specular map; u_lightDir, u_lightColor).

// No tangents in the vertex: the frame comes from how position and uv
// change across the screen (a cotangent frame). GTA's maps are
// DirectX's, green towards +v, which is the frame's own direction.
vec3 BumpedNormal(vec3 N) {
    if (u_material.x < 0.5) return N;
    vec2 xy = texture(bumpTex, v_uv).rg * 2.0 - 1.0;
    vec3 m = vec3(xy, sqrt(max(1.0 - dot(xy, xy), 0.0)));
    vec3 dp1 = dFdx(v_worldPos), dp2 = dFdy(v_worldPos);
    vec2 duv1 = dFdx(v_uv), duv2 = dFdy(v_uv);
    vec3 dp2perp = cross(dp2, N), dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    float scale = inversesqrt(max(max(dot(T, T), dot(B, B)), 1e-20));
    return normalize(mat3(T * scale, B * scale, N) * m);
}

float SpecularMask() {
    if (u_material.y < 0.5) return 0.0;
    return dot(texture(specTex, v_uv).rgb, vec3(1.0 / 3.0));
}

// Blinn-Phong for the sun, as bright as the map allows, in the shadow's
// light only. Unscaled by exposure: the caller does that.
vec3 Specular(vec3 N, float mask, float shadow) {
    if (mask <= 0.0) return vec3(0.0);
    vec3 L = normalize(-u_lightDir.xyz);
    vec3 V = normalize(v_cameraPos - v_worldPos);
    vec3 H = normalize(L + V);
    float s = pow(max(dot(N, H), 0.0), 48.0) * max(dot(N, L), 0.0);
    return u_lightColor.rgb * s * mask * shadow * 0.6;
}
