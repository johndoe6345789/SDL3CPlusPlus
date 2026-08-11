#include <metal_stdlib>
using namespace metal;

struct TessVertex {
    packed_float3 position;
    packed_float2 uv;
};

struct TessParams {
    float width;
    float depth;
    float displacement_strength;
    float uv_scale_x;
    float uv_scale_y;
    uint subdivisions;
    uint _pad0;
    uint _pad1;
};

kernel void main0(
    texture2d<float> displacement [[texture(0)]],
    sampler dispSampler [[sampler(0)]],
    device TessVertex* vertices [[buffer(0)]],
    constant TessParams& params [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]])
{
    uint subdiv = params.subdivisions;
    if (gid.x > subdiv || gid.y > subdiv) return;

    float u = float(gid.x) / float(subdiv);
    float v = float(gid.y) / float(subdiv);

    // Sample displacement with bilinear interpolation
    float disp = displacement.sample(dispSampler, float2(u, v)).r;

    // Generate vertex on XZ plane, displaced along Y
    float hw = params.width * 0.5;
    float hd = params.depth * 0.5;

    uint idx = gid.y * (subdiv + 1) + gid.x;
    vertices[idx].position = float3(
        -hw + u * params.width,
        disp * params.displacement_strength,
        -hd + v * params.depth
    );
    vertices[idx].uv = float2(u * params.uv_scale_x, v * params.uv_scale_y);
}
