#version 450

// BL4 static mesh shading: one placeholder texture (bl4x doesn't resolve
// UMaterialInstance -> texture parameters yet, so every mesh currently
// draws with bl4_placeholder.png) tinted by one sun direction and a flat
// sky-ambient term. Deliberately simple compared to gta5_model.frag --
// this is the first walkable slice, not the final look.

layout(set = 2, binding = 0) uniform sampler2D albedoTex;

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec4 u_sunDir;    // xyz = direction the light travels
    vec4 u_sunColor;  // rgb = colour * intensity
    vec4 u_ambient;   // rgb = sky colour * intensity
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 0) out vec4 o_color;

void main() {
    vec3 albedo = texture(albedoTex, v_uv).rgb;
    vec3 n = normalize(v_worldNormal);
    if (!gl_FrontFacing) n = -n;
    float sun = max(dot(n, -normalize(u_sunDir.xyz)), 0.0);
    vec3 color = albedo * (u_sunColor.rgb * sun + u_ambient.rgb);
    o_color = vec4(color, 1.0);
}
