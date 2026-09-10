#include "services/interfaces/workflow/graphics/gpu_pipeline_vertex_layout.hpp"

namespace sdl3cpp::services::impl {

GpuVertexAttributeLayout BuildVertexAttributeLayout(
    const std::string& vertexFormat) {
    GpuVertexAttributeLayout layout;
    auto& vbuf              = layout.vbufDesc;
    auto& attrs             = layout.attrs;
    vbuf.slot               = 0;
    vbuf.input_rate         = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vbuf.instance_step_rate = 0;
    attrs[0].location       = 0;
    attrs[0].buffer_slot    = 0;
    attrs[0].format         = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset         = 0;

    if (vertexFormat == "none") {
        // Fullscreen triangle: no vertex buffers, vertex_id only.
        layout.numBuffers    = 0;
        layout.numAttributes = 0;
    } else if (vertexFormat == "position_uv_lmuv_normal") {
        // BSP: float3 pos + float2 uv + float2 lmuv + float3 normal = 40B.
        vbuf.pitch           = sizeof(float) * 10;
        attrs[1].location    = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[1].offset      = sizeof(float) * 3;  // 12
        attrs[2].location    = 2;
        attrs[2].buffer_slot = 0;
        attrs[2].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[2].offset      = sizeof(float) * 5;  // 20
        attrs[3].location    = 3;
        attrs[3].buffer_slot = 0;
        attrs[3].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        attrs[3].offset      = sizeof(float) * 7;  // 28
        layout.numBuffers    = 1;
        layout.numAttributes = 4;
    } else if (vertexFormat == "position_uv") {
        // Textured: float3 position + float2 uv = 20 bytes.
        vbuf.pitch           = sizeof(float) * 5;
        attrs[1].location    = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[1].offset      = sizeof(float) * 3;
        layout.numBuffers    = 1;
        layout.numAttributes = 2;
    } else {
        // Default position_color: float3 position + ubyte4 color = 16B.
        vbuf.pitch           = sizeof(float) * 3 + sizeof(uint8_t) * 4;
        attrs[1].location    = 1;
        attrs[1].buffer_slot = 0;
        attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        attrs[1].offset      = sizeof(float) * 3;
        layout.numBuffers    = 1;
        layout.numAttributes = 2;
    }
    return layout;
}

}  // namespace sdl3cpp::services::impl
