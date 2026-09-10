#include "services/interfaces/workflow/graphics/frame_swapchain.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

SDL_GPUCommandBuffer* AcquireFrameCommandBufferOrThrow(SDL_GPUDevice* device) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        throw std::runtime_error(
            "graphics.frame.begin: SDL_AcquireGPUCommandBuffer failed: " +
            std::string(SDL_GetError()));
    }
    return cmd;
}

SwapchainAcquireResult AcquireSwapchainTextureOrThrow(SDL_GPUCommandBuffer* cmd,
                                                      SDL_Window* window) {
    SwapchainAcquireResult result{nullptr, 0, 0};
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &result.texture,
                                               &result.width, &result.height)) {
        SDL_CancelGPUCommandBuffer(cmd);
        throw std::runtime_error(
            "graphics.frame.begin: "
            "SDL_WaitAndAcquireGPUSwapchainTexture failed: " +
            std::string(SDL_GetError()));
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
