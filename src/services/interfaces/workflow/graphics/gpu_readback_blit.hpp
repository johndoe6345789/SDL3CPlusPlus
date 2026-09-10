#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// A staging texture holding a blitted copy of the swapchain, sized to
/// match the swapchain's actual (not the window's requested) dimensions.
struct BlittedSwapchainStaging {
    SDL_GPUTexture* texture = nullptr;
    uint32_t width          = 0;
    uint32_t height         = 0;
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

}  // namespace sdl3cpp::services::impl
