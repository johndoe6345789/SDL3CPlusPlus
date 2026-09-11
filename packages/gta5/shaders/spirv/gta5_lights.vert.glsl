#version 450

// GTA's distant lights: each a square about its centre, turned to face
// the camera and kept a few pixels across however far off it is -- a
// lamp a kilometre away is still a point of light. Vertex format
// position_uv_lmuv_normal: the centre, the corner (-1..1), the intensity
// in lm_u, the colour in the normal.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_lmuv;
layout(location = 3) in vec3 a_normal;

layout(set = 1, binding = 0) uniform LightUniforms {
    mat4 u_viewProj;
    vec4 u_right;   // the camera's right and up, in world space
    vec4 u_up;
    vec4 u_camera;
    vec4 u_params;  // night, metres of size a metre away, fade from, to
};

layout(location = 0) out vec2 v_corner;
layout(location = 1) out vec4 v_colour;  // rgb, a = brightness

void main() {
    float dist = length(a_position - u_camera.xyz);
    float size = max(0.35, dist * u_params.y);
    vec3 world = a_position +
                 (u_right.xyz * a_uv.x + u_up.xyz * a_uv.y) * size;
    gl_Position = u_viewProj * vec4(world, 1.0);
    // Up close GTA lights the lamp itself; here that is a blob, so fade.
    float far = smoothstep(u_params.z, u_params.w, dist);
    v_corner = a_uv;
    v_colour = vec4(a_normal, a_lmuv.x * far * u_params.x);
}
