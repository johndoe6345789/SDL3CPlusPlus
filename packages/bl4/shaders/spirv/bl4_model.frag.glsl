#version 450

// BL4 static mesh shading: the base-colour map bl4x resolved from each
// material instance (bl4_placeholder.png where it found none), lit by
// one sun and a sky term the way gta5_shade.glsl lights GTA's map.
// BL4's materials are layered graphs (tint, grime, wear, detail maps);
// only the base colour survives here.

layout(set = 2, binding = 0) uniform sampler2D albedoTex;

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec4 u_sunDir;    // xyz = direction the light travels
    vec4 u_sunColor;  // rgb = colour * intensity, a = exposure
    vec4 u_ambient;   // rgb = sky colour * intensity
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 0) out vec4 o_color;

// Colour maps hold display values; lighting needs them linear, and the
// composite encodes once at the end. Unconverted, every surface under
// the desert sun washed out to white.
vec3 SrgbToLinear(vec3 c) {
    vec3 lo = c / 12.92;
    vec3 hi = pow((c + 0.055) / 1.055, vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), c));
}

void main() {
    vec3 albedo = SrgbToLinear(texture(albedoTex, v_uv).rgb);
    vec3 n = normalize(v_worldNormal);
    if (!gl_FrontFacing) n = -n;
    float sun = max(dot(n, -normalize(u_sunDir.xyz)), 0.0);
    float sky = mix(0.55, 1.0, n.y * 0.5 + 0.5);
    float exposure = u_sunColor.a > 0.0 ? u_sunColor.a : 1.0;
    vec3 lit = albedo * (u_sunColor.rgb * sun + u_ambient.rgb * sky);
    o_color = vec4(lit * exposure, 1.0);
}
