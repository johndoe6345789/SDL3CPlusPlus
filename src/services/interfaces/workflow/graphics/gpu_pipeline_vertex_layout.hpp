#pragma once

#include <SDL3/SDL_gpu.h>
#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/// Vertex buffer/attribute storage for one vertex layout, owned by the
/// caller so pointers into it stay valid until pipeline creation.
struct GpuVertexAttributeLayout {
    SDL_GPUVertexBufferDescription vbufDesc     = {};
    std::array<SDL_GPUVertexAttribute, 4> attrs = {};
    Uint32 numBuffers                           = 0;
    Uint32 numAttributes                        = 0;
};

/**
 * @brief Builds the vertex buffer/attribute layout for a named format.
 *
 * Supported formats: "none" (fullscreen triangle, no vertex buffers),
 * "position_uv_lmuv_normal" (BSP: pos+uv+lmuv+normal, 40 bytes),
 * "position_uv" (pos+uv, 20 bytes), and the default "position_color"
 * (pos+ubyte4 color, 16 bytes).
 */
GpuVertexAttributeLayout BuildVertexAttributeLayout(
    const std::string& vertexFormat);

}  // namespace sdl3cpp::services::impl
