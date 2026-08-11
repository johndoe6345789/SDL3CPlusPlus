#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct TessVertex {
    float px, py, pz; // position
    float u, v;       // uv
};

layout(set = 0, binding = 0) uniform sampler2D displacement;

layout(std430, set = 0, binding = 1) buffer VertexBuffer {
    TessVertex vertices[];
};

layout(set = 0, binding = 2) uniform TessParams {
    float width;
    float depth;
    float displacement_strength;
    float uv_scale_x;
    float uv_scale_y;
    uint subdivisions;
    uint _pad0;
    uint _pad1;
};

void main() {
    uvec2 gid = gl_GlobalInvocationID.xy;
    uint subdiv = subdivisions;
    if (gid.x > subdiv || gid.y > subdiv) return;

    float uf = float(gid.x) / float(subdiv);
    float vf = float(gid.y) / float(subdiv);

    // Sample displacement with bilinear interpolation
    float disp = texture(displacement, vec2(uf, vf)).r;

    // Generate vertex on XZ plane, displaced along Y
    float hw = width * 0.5;
    float hd = depth * 0.5;

    uint idx = gid.y * (subdiv + 1) + gid.x;
    vertices[idx].px = -hw + uf * width;
    vertices[idx].py = disp * displacement_strength;
    vertices[idx].pz = -hd + vf * depth;
    vertices[idx].u  = uf * uv_scale_x;
    vertices[idx].v  = vf * uv_scale_y;
}
