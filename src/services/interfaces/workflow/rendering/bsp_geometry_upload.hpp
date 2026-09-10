#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// The GPU vertex/index buffers produced by uploading BSP geometry.
struct BspGeometryBuffers {
    SDL_GPUBuffer* vertex_buffer = nullptr;
    SDL_GPUBuffer* index_buffer  = nullptr;
};

/**
 * @brief Creates a GPU vertex buffer and index buffer sized for
 *        `vertices`/`indices`, then uploads both through a single
 *        shared transfer buffer.
 *
 * The transfer buffer holds the vertex data followed by the index
 * data back-to-back, matching the two-region copy pass the caller
 * previously performed inline.
 */
BspGeometryBuffers UploadBspGeometryBuffers(
    SDL_GPUDevice* device, const std::vector<BspRenderVertex>& vertices,
    const std::vector<uint32_t>& indices);

}  // namespace sdl3cpp::services::impl
