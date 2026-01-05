#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneColor;
layout(set = 0, binding = 1) uniform sampler2D depthBuffer;

layout(push_constant) uniform PushConstants {
    vec2 lightScreenPos;
    float time;
    float intensity;
} pc;

const int NUM_SAMPLES = 100;
const float DECAY_BASE = 0.96815;
const float WEIGHT_BASE = 0.58767;
const float EXPOSURE = 0.2;

void main() {
    vec2 texCoord = inTexCoord;
    vec2 deltaTexCoord = (texCoord - pc.lightScreenPos);
    deltaTexCoord *= 1.0 / float(NUM_SAMPLES) * 0.5;  // Scale for effect

    vec3 color = texture(sceneColor, texCoord).rgb;

    // Only apply god rays if we're looking towards the light
    float centerDistance = length(pc.lightScreenPos - vec2(0.5, 0.5));
    if (centerDistance < 0.8) {  // Light is visible on screen
        vec3 godRayColor = vec3(0.0);
        float weight = WEIGHT_BASE;

        for (int i = 0; i < NUM_SAMPLES; i++) {
            texCoord -= deltaTexCoord;
            vec3 sampleColor = texture(sceneColor, texCoord).rgb;
            godRayColor += sampleColor * weight;
            weight *= DECAY_BASE;
        }

        color += godRayColor * EXPOSURE * pc.intensity;
    }

    outColor = vec4(color, 1.0);
}