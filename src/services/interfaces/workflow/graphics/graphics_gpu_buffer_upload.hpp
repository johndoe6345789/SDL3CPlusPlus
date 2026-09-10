#pragma once

#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_create.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Creates a vertex buffer and an index buffer on `device`, sized to hold
/// `vertexBytes` and `indexValues`, and uploads both through a single
/// shared transfer buffer. Throws std::runtime_error (and releases any
/// buffer it already created) if any GPU call fails.
UploadedGpuBuffers CreateAndUploadGpuBuffers(
    SDL_GPUDevice* device, const std::vector<uint8_t>& vertexBytes,
    const std::vector<uint16_t>& indexValues);

/// Builds the "cube_mesh" metadata blob describing the uploaded buffers.
nlohmann::json BuildUploadedMeshMetadata(int vertexCount, int indexCount,
                                         int vertexStride);

}  // namespace sdl3cpp::services::impl
