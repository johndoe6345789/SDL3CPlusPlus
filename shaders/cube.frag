#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

vec3 RainbowBand(float t) {
    t = clamp(t, 0.0, 1.0);
    float scaled = t * 5.0;
    int index = int(floor(scaled));
    float blend = fract(scaled);

    vec3 red = vec3(0.91, 0.19, 0.21);
    vec3 orange = vec3(0.99, 0.49, 0.09);
    vec3 yellow = vec3(0.99, 0.86, 0.22);
    vec3 green = vec3(0.16, 0.74, 0.39);
    vec3 blue = vec3(0.24, 0.48, 0.88);
    vec3 purple = vec3(0.56, 0.25, 0.75);

    vec3 a = red;
    vec3 b = orange;
    if (index == 1) {
        a = orange;
        b = yellow;
    } else if (index == 2) {
        a = yellow;
        b = green;
    } else if (index == 3) {
        a = green;
        b = blue;
    } else if (index == 4) {
        a = blue;
        b = purple;
    } else if (index >= 5) {
        a = purple;
        b = purple;
    }

    return mix(a, b, blend);
}

void main() {
    float bandPos = (fragWorldPos.y + 1.0) * 0.5;
    float diagonal = (fragWorldPos.x + fragWorldPos.z) * 0.15;
    vec3 rainbow = RainbowBand(bandPos + diagonal);
    vec3 shaded = mix(rainbow, fragColor, 0.08);
    outColor = vec4(shaded, 1.0);
}
