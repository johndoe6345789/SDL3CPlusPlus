#include <metal_stdlib>
using namespace metal;

struct VertexOutput {
    float4 position [[position]];
    float2 uv;
};

vertex VertexOutput main0(uint vid [[vertex_id]])
{
    VertexOutput out;
    // Fullscreen triangle: 3 vertices covering [-1,1] clip space
    out.uv = float2((vid << 1) & 2, vid & 2);
    out.position = float4(out.uv * 2.0 - 1.0, 0.0, 1.0);
    // Flip Y for Metal's top-left origin
    out.uv.y = 1.0 - out.uv.y;
    return out;
}
