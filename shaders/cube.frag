#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

vec3 ComputeNormal() {
    vec3 dx = dFdx(fragWorldPos);
    vec3 dy = dFdy(fragWorldPos);
    return normalize(cross(dx, dy));
}

vec3 RainbowBand(float t) {
    t = fract(t);
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
    float bandPos = fragWorldPos.y * 0.35;
    float diagonal = (fragWorldPos.x + fragWorldPos.z) * 0.25;
    vec3 rainbow = RainbowBand(bandPos + diagonal);
    vec3 baseColor = mix(rainbow, fragColor, 0.08);

    vec3 normal = ComputeNormal();
    vec3 lighting = vec3(0.0);

    vec3 keyDir = normalize(vec3(-0.35, 1.0, -0.25));
    vec3 fillDir = normalize(vec3(0.45, 0.6, 0.2));
    float keyNdotL = abs(dot(normal, keyDir));
    float fillNdotL = abs(dot(normal, fillDir));

    lighting += vec3(1.0, 0.92, 0.85) * keyNdotL * 0.9;
    lighting += vec3(0.35, 0.45, 0.65) * fillNdotL * 0.4;

    vec3 finalColor = baseColor * (0.28 + lighting);
    outColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
