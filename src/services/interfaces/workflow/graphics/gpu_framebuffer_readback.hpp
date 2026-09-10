#pragma once

#include "services/interfaces/workflow/graphics/gpu_readback_blit.hpp"
#include "services/interfaces/workflow/graphics/gpu_readback_keys.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Downloads `staging` to a CPU buffer and releases it.
 *
 * Creates a download transfer buffer, copies `staging` into it on its own
 * command buffer, blocks on a fence for GPU completion, maps and copies
 * the pixels out, then releases the transfer buffer and `staging.texture`
 * regardless of outcome.
 *
 * @return The RGBA8 pixel data, or an empty vector on any failure.
 */
std::vector<uint8_t> DownloadStagingTexture(
    SDL_GPUDevice* device, const BlittedSwapchainStaging& staging);

}  // namespace sdl3cpp::services::impl
