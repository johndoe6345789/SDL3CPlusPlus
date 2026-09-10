#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A staging texture holding a blitted copy of the swapchain, sized to
/// match the swapchain's actual (not the window's requested) dimensions.
struct BlittedSwapchainStaging {
    SDL_GPUTexture* texture = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
};

/**
 * @brief Acquires the swapchain and blits it into a new staging texture.
 *
 * Submits its own command buffer. Returns a null-textured result (with
 * width/height left at 0) if the swapchain texture could not be acquired
 * or the staging texture could not be created — the caller then has
 * nothing further to release.
 */
BlittedSwapchainStaging BlitSwapchainToStaging(SDL_GPUDevice* device,
                                               SDL_Window* window);

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
