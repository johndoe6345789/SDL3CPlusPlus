#include "services/interfaces/workflow/rendering/frame_swapchain_acquire.hpp"

namespace sdl3cpp::services::impl {

AcquiredSwapchain AcquireSwapchainForFrame(SDL_GPUDevice* device,
                                           SDL_Window* window) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        return {};
    }

    SDL_GPUTexture* swapchainTex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTex, &sw,
                                               &sh) ||
        !swapchainTex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return {};
    }

    return AcquiredSwapchain{cmd, swapchainTex, sw, sh, true};
}

}  // namespace sdl3cpp::services::impl
