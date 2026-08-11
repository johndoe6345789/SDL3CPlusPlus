#version 450

layout(set = 3, binding = 0) uniform BloomParams {
    vec4 params; // x=threshold, y=soft_knee, z=0, w=0
};

layout(set = 2, binding = 0) uniform sampler2D hdr_texture;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

void main() {
    vec3 color = texture(hdr_texture, v_uv).rgb;

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float threshold = params.x;
    float knee = params.y;

    // Soft knee: smooth quadratic falloff near threshold
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);

    float contribution = max(soft, brightness - threshold);
    contribution /= max(brightness, 0.00001);

    o_color = vec4(color * max(contribution, 0.0), 1.0);
}
