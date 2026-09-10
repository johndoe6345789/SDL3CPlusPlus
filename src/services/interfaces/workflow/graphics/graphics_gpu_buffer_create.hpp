#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// The pair of GPU buffers created and populated by
/// CreateAndUploadGpuBuffers.
struct UploadedGpuBuffers {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer  = nullptr;
};

/// Creates an empty vertex buffer of `vertexSize` bytes and an index buffer
/// of `indexSize` bytes on `device`. Throws std::runtime_error (and
/// releases the vertex buffer first) if either creation fails.
UploadedGpuBuffers CreateGpuVertexIndexBuffers(SDL_GPUDevice* device,
                                               uint32_t vertexSize,
                                               uint32_t indexSize);

}  // namespace sdl3cpp::services::impl
