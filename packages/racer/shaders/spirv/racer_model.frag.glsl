#version 450

// Texture modulated by the baked vertex colour (the N64 combiner's
// TEXEL0 * SHADE), cut out where the texture is transparent, then faded
// into distance fog. The original drew a short fog wall; this version
// keeps the full draw distance and uses fog only for atmosphere.

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec4 u_fogColour;   // rgb, a = fog start distance
    vec4 u_fogParams;   // x = full-fog distance, y = alpha cutoff,
                        // z = 1 for the sky: fade it into the fog colour
                        //     toward and below the horizon
    vec4 u_cameraPos;
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_colour;
layout(location = 2) in vec3 v_worldPos;
layout(location = 0) out vec4 o_colour;

void main() {
    vec4 texel = texture(u_texture, v_uv);
    vec4 colour = texel * v_colour;
    if (colour.a < u_fogParams.y * 0.5) discard;
    float distance = length(v_worldPos - u_cameraPos.xyz);
    float fog = clamp((distance - u_fogColour.a) /
                      max(u_fogParams.x - u_fogColour.a, 1.0), 0.0, 1.0);
    // The skybox is a band; its lower edge would show as a hard line
    // against the clear colour, so the sky melts into the fog (which is
    // also the clear colour) as the view drops to the horizon.
    vec3 view = normalize(v_worldPos - u_cameraPos.xyz);
    float horizon = 1.0 - smoothstep(0.0, 0.35, view.y);
    fog = max(fog, u_fogParams.z * horizon);
    o_colour = vec4(mix(colour.rgb, u_fogColour.rgb, fog * fog), colour.a);
}
