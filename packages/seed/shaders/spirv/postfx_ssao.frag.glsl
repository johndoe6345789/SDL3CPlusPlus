#version 450

const float PI = 3.14159265359;

layout(set = 3, binding = 0) uniform SSAOParams {
    mat4  u_projection;
    mat4  u_inv_projection;
    vec4  u_params;       // x=radius, y=bias, z=1/width, w=1/height
    vec4  u_kernel[16];   // hemisphere sample directions
};

layout(set = 2, binding = 0) uniform sampler2D depthTex;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

vec3 reconstructViewPos(vec2 uv, float depth, mat4 invProj) {
    // UV to clip space [-1, 1]
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    clip.y = -clip.y;  // Vulkan NDC convention
    vec4 viewPos = invProj * clip;
    return viewPos.xyz / viewPos.w;
}

// Screen-space procedural noise for sample rotation
vec2 noiseFromUV(vec2 uv, vec2 texelSize) {
    vec2 screenPos = uv / texelSize;
    return fract(sin(vec2(
        dot(screenPos, vec2(12.9898, 78.233)),
        dot(screenPos, vec2(39.346, 11.135))
    )) * 43758.5453);
}

void main() {
    float depth = texture(depthTex, v_uv).r;

    // Sky pixels: no occlusion
    if (depth >= 1.0) {
        o_color = vec4(1.0);
        return;
    }

    // TODO: SSAO needs Vulkan clip-space tuning — bypass for now
    o_color = vec4(1.0);
    return;

    float radius = u_params.x;
    float bias   = u_params.y;
    vec2 texelSize = u_params.zw;

    // Reconstruct view-space position and normal
    vec3 fragPos = reconstructViewPos(v_uv, depth, u_inv_projection);
    vec3 normal  = normalize(cross(dFdx(fragPos), dFdy(fragPos)));

    // Random rotation from procedural noise
    vec2 noise = noiseFromUV(v_uv, texelSize);
    float angle = noise.x * 2.0 * PI;
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Build TBN from normal + random tangent
    vec3 randomVec = vec3(cosA, sinA, 0.0);
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < 16; ++i) {
        // Orient sample in hemisphere around normal
        vec3 sampleDir = TBN * u_kernel[i].xyz;
        vec3 samplePos = fragPos + sampleDir * radius;

        // Project sample to screen space
        vec4 offset = u_projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        vec2 sampleUV = offset.xy * 0.5 + 0.5;
        sampleUV.y = 1.0 - sampleUV.y;

        // Sample depth at projected position
        float sampleDepth = texture(depthTex, sampleUV).r;
        vec3 sampleViewPos = reconstructViewPos(sampleUV, sampleDepth, u_inv_projection);

        // Range check: only occlude if sample is within radius
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - sampleViewPos.z), 0.001));
        occlusion += (sampleViewPos.z >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    float ao = 1.0 - (occlusion / 16.0);
    o_color = vec4(ao, ao, ao, 1.0);
}
