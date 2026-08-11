#version 450

layout(set = 3, binding = 0) uniform BlurParams {
    vec4 direction; // xy = texel-scaled blur direction
};

layout(set = 2, binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

void main() {
    vec2 dir = direction.xy;

    // 9-tap Gaussian (sigma ~= 4, weights sum to 1.0)
    float weight[5] = float[5](0.227027027, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

    vec3 result = texture(source, v_uv).rgb * weight[0];

    for (int i = 1; i < 5; ++i) {
        vec2 offset = dir * float(i);
        result += texture(source, v_uv + offset).rgb * weight[i];
        result += texture(source, v_uv - offset).rgb * weight[i];
    }

    o_color = vec4(result, 1.0);
}
