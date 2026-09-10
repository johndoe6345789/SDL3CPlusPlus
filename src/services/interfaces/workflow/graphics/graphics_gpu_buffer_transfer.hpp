#pragma once

#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_create.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Copies `vertexBytes`/`indexValues` into `buffers` through a single
/// shared transfer buffer. Throws std::runtime_error if the transfer
/// buffer can't be created; `buffers` are left for the caller to release
/// in that case.
void UploadGpuBufferData(SDL_GPUDevice* device,
                         const UploadedGpuBuffers& buffers,
                         const std::vector<uint8_t>& vertexBytes,
                         const std::vector<uint16_t>& indexValues);

}  // namespace sdl3cpp::services::impl
