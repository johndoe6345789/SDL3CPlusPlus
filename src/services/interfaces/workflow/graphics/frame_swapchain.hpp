#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Acquires the GPU command buffer for a new frame.
 * @throws std::runtime_error (naming SDL_GetError()) on failure.
 */
SDL_GPUCommandBuffer* AcquireFrameCommandBufferOrThrow(SDL_GPUDevice* device);

/// Swapchain texture plus its current size, or a null texture if the
/// window is minimized/not visible and the frame should be skipped.
struct SwapchainAcquireResult {
    SDL_GPUTexture* texture;
    Uint32 width;
    Uint32 height;
};

/**
 * @brief Waits for and acquires the swapchain texture for `cmd`.
 * @throws std::runtime_error (naming SDL_GetError(), cancelling `cmd`
 * first) if the underlying SDL call fails outright.
 */
SwapchainAcquireResult AcquireSwapchainTextureOrThrow(SDL_GPUCommandBuffer* cmd,
                                                      SDL_Window* window);

}  // namespace sdl3cpp::services::impl
