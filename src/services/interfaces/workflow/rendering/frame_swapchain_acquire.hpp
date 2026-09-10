#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// The swapchain image acquired for this frame, plus the command buffer
/// it was acquired on.
struct AcquiredSwapchain {
    SDL_GPUCommandBuffer* cmd = nullptr;
    SDL_GPUTexture* texture   = nullptr;
    uint32_t width = 0, height = 0;
    bool ok = false;
};

/**
 * @brief Acquires a command buffer and this frame's swapchain texture.
 *
 * On failure (no command buffer, or the swapchain isn't ready), submits
 * whatever command buffer was acquired and returns ok=false; the caller
 * should set frame_skip and return without touching `cmd` further.
 */
AcquiredSwapchain AcquireSwapchainForFrame(SDL_GPUDevice* device,
                                           SDL_Window* window);

}  // namespace sdl3cpp::services::impl
