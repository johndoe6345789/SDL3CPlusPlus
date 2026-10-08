#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 2) in vec3 v_worldPos;

layout(location = 0) out vec4 o_color;

const vec3 kRed = vec3(0.86, 0.10, 0.08);
const vec3 kWhite = vec3(0.96, 0.96, 0.92);
const vec3 kBlack = vec3(0.04, 0.04, 0.05);
const vec3 kGold = vec3(0.95, 0.75, 0.25);
// Red and white bands 1.2 m tall on the posts; 1.2 m checker on the banner.
const float kBandHeight = 1.2;
const float kCheckSize = 1.2;

void main() {
    float shade = 0.45 + 0.55 * v_uv.x;
    vec3 base;
    if (v_uv.y > 0.5) {
        float cell = mod(floor(v_worldPos.x / kCheckSize) +
                         floor(v_worldPos.y / kCheckSize), 2.0);
        base = mix(kBlack, kWhite, cell);
    } else {
        float band = step(0.5, fract(v_worldPos.y / kBandHeight));
        base = mix(kRed, kWhite, band);
    }
    o_color = vec4(base * shade + 0.08, 1.0);
}
