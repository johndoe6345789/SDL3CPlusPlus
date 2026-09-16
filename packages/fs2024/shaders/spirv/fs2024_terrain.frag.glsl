#version 450

// FS2024 ground shading: the baked ground map, lit by one sun and a sky
// term, with ground-level detail, the runway's markings and distance fog.
//
// The ground map is about 2 m a texel, which up close is a blur. Two
// octaves of world-space value noise break it up underfoot and fade out
// with distance, where the map alone reads right. The runway is drawn
// over it analytically (fs2024_runway.glsl): the map only carries its
// asphalt, for the distance.

layout(set = 2, binding = 0) uniform sampler2D ground_map;

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec4 u_sunDir;      // direction the light travels
    vec4 u_sunColour;   // rgb = colour * intensity, a = exposure
    vec4 u_ambient;     // rgb = sky colour * intensity
    vec4 u_fog;         // rgb = linear horizon colour, a = density per m
    vec4 u_runway;      // centre x, z; half length, half width (0 = none)
    vec4 u_runwayAxis;  // xy = unit x, z down the runway
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_cameraPos;
layout(location = 0) out vec4 o_color;

#include "include/fs2024_runway.glsl"

// Hoskins' hash without sine: sin() loses precision at world-scale
// inputs and the noise went to diagonal blocks across the asphalt.
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}

void main() {
    // The map is an sRGB JPEG uploaded as UNORM: decode here.
    vec3 albedo = pow(texture(ground_map, v_uv).rgb, vec3(2.2));

    float distance = length(v_worldPos - v_cameraPos);
    float detail = valueNoise(v_worldPos.xz * 0.9) * 0.6 +
                   valueNoise(v_worldPos.xz * 3.7) * 0.4;
    float nearby = 1.0 - smoothstep(20.0, 160.0, distance);
    albedo *= mix(1.0, 0.75 + 0.5 * detail, nearby);

    // Asphalt and paint get a finer, fainter grain than grass.
    float inside;
    vec3 runway = runwaySurface(v_worldPos, inside);
    float grain = valueNoise(v_worldPos.xz * 6.0);
    runway *= mix(1.0, 0.88 + 0.24 * grain, nearby);
    albedo = mix(albedo, runway, inside);

    vec3 n = normalize(v_normal);
    float sun = max(dot(n, -normalize(u_sunDir.xyz)), 0.0);
    float sky = 0.5 + 0.5 * n.y;
    vec3 lit = albedo * (u_sunColour.rgb * sun + u_ambient.rgb * sky);
    lit *= u_sunColour.a > 0.0 ? u_sunColour.a : 1.0;

    // Exponential fog, so the far mountains sink into the horizon.
    float fog = 1.0 - exp(-distance * u_fog.a);
    o_color = vec4(mix(lit, u_fog.rgb, fog), 1.0);
}
