#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneColor;
layout(set = 0, binding = 1) uniform sampler2D normalBuffer;
layout(set = 0, binding = 2) uniform sampler2D depthBuffer;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

const int NUM_SAMPLES = 16;
const float SAMPLE_RADIUS = 0.5;

// Reconstruct world position from depth
vec3 worldPosFromDepth(float depth, vec2 texCoord) {
    vec4 clipSpace = vec4(texCoord * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpace = pc.proj * clipSpace;  // Use proj as invProj for now (dummy)
    viewSpace /= viewSpace.w;
    vec4 worldSpace = pc.view * viewSpace;  // Use view as invView for now (dummy)
    return worldSpace.xyz;
}

vec3 ssao(vec2 texCoord) {
    float depth = texture(depthBuffer, texCoord).r;
    if (depth >= 1.0) return vec3(1.0);  // Skybox

    vec3 worldPos = worldPosFromDepth(depth, texCoord);
    vec3 normal = normalize(texture(normalBuffer, texCoord).rgb * 2.0 - 1.0);

    float occlusion = 0.0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        // Generate sample position in hemisphere around normal
        float angle = (float(i) / float(NUM_SAMPLES)) * 6.283185;
        vec2 offset = vec2(cos(angle), sin(angle)) * SAMPLE_RADIUS;

        vec2 sampleTexCoord = texCoord + offset / textureSize(depthBuffer, 0);
        float sampleDepth = texture(depthBuffer, sampleTexCoord).r;

        if (sampleDepth < depth - 0.01) {  // Occluded
            occlusion += 1.0;
        }
    }

    occlusion = 1.0 - (occlusion / float(NUM_SAMPLES));
    return vec3(occlusion);
}

void main() {
    vec3 sceneColor = texture(sceneColor, inTexCoord).rgb;
    vec3 ao = ssao(inTexCoord);

    // Apply ambient occlusion
    vec3 finalColor = sceneColor * (0.3 + 0.7 * ao);  // Mix AO with direct lighting

    outColor = vec4(finalColor, 1.0);
}