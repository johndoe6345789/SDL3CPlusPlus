#version 450

// FS2024 ground the way the simulator textures it offline: each tile's
// own land-class map (from the game's ground-cover layer) picks a
// surface out of the game's own material array (bf-texture-synth-lib's
// array_low.dds), tiled in world space. The four class texels around a
// point are blended by their bilinear weights, so a field meets a town
// along a soft edge rather than a stair of 24 m squares. Each class
// texel draws one of its material's own variations, chosen by a hash of
// where it is, so a whole county of grassland is not one repeated tile.
// Lit and fogged exactly as fs2024_terrain.frag lights its buildings.

layout(set = 2, binding = 0) uniform sampler2D class_map;        // R8
layout(set = 2, binding = 1) uniform sampler2DArray materials;   // BC1 sRGB

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec4 u_sunDir;       // direction the light travels
    vec4 u_sunColour;    // rgb = colour * intensity, a = exposure
    vec4 u_ambient;      // rgb = sky colour * intensity
    vec4 u_fog;          // rgb = linear horizon colour, a = density per m
    vec4 u_material;     // x = metres per material repeat, y = map size
    vec4 u_table[16];    // per land class: x = first layer, y = layers
};

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_cameraPos;
layout(location = 0) out vec4 o_color;

// Hoskins' hash without sine: sin() loses precision at world scale.
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 surfaceAt(ivec2 texel, vec2 repeatUv) {
    int size = int(u_material.y);
    ivec2 at = clamp(texel, ivec2(0), ivec2(size - 1));
    int landClass = int(texelFetch(class_map, at, 0).r * 255.0 + 0.5);
    vec4 entry = u_table[clamp(landClass, 0, 15)];
    // Seed by world position, not tile-local texel, so neighbouring
    // tiles pick independently rather than repeating one pattern.
    vec2 cell = floor(v_worldPos.xz / (u_material.x * 0.5)) + vec2(at);
    float variation = floor(hash(cell) * max(entry.y, 1.0));
    return texture(materials, vec3(repeatUv, entry.x + variation)).rgb;
}

void main() {
    float size = u_material.y;
    vec2 t = v_uv * size - 0.5;
    ivec2 base = ivec2(floor(t));
    vec2 f = fract(t);
    vec2 repeatUv = v_worldPos.xz / u_material.x;
    vec3 albedo = mix(
        mix(surfaceAt(base, repeatUv), surfaceAt(base + ivec2(1, 0), repeatUv),
            f.x),
        mix(surfaceAt(base + ivec2(0, 1), repeatUv),
            surfaceAt(base + ivec2(1, 1), repeatUv), f.x),
        f.y);

    vec3 n = normalize(v_normal);
    float sun = max(dot(n, -normalize(u_sunDir.xyz)), 0.0);
    float sky = 0.5 + 0.5 * n.y;
    vec3 lit = albedo * (u_sunColour.rgb * sun + u_ambient.rgb * sky);
    lit *= u_sunColour.a > 0.0 ? u_sunColour.a : 1.0;

    float distance = length(v_worldPos - v_cameraPos);
    float fog = 1.0 - exp(-distance * u_fog.a);
    o_color = vec4(mix(lit, u_fog.rgb, fog), 1.0);
}
