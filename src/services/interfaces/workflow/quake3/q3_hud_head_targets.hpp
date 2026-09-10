#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Soft front-right key light + ambient, matching the original portrait.
rendering::FragmentUniformData DefaultHeadPortraitLighting();

/// Color + depth render targets used to render the head portrait offscreen.
struct HeadRenderTargets {
    SDL_GPUTexture* color = nullptr;
    SDL_GPUTexture* depth = nullptr;
    bool ready            = false;
};

/**
 * @brief Creates the `size` x `size` color/depth render targets.
 *
 * The color target's format matches the swapchain's so the caller's
 * textured pipeline (compiled for the swapchain) can render into it
 * without a format-mismatch error.
 */
HeadRenderTargets CreateHeadRenderTargets(SDL_GPUDevice* device,
                                          SDL_Window* window, int size);

}  // namespace sdl3cpp::services::impl
