#include <metal_stdlib>
using namespace metal;

// Minimal fragment shader for depth-only pass (Metal requires a fragment stage)
fragment float4 main0() {
    return float4(0.0);
}
