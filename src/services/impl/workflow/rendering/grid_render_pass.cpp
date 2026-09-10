#include "services/interfaces/workflow/rendering/grid_render_pass.hpp"

namespace sdl3cpp::services::impl {

SDL_GPURenderPass* BeginGridRenderPass(SDL_GPUCommandBuffer* cmd,
                                       const GridGpuResources& gpu,
                                       const GridDrawConfig& cfg) {
    SDL_GPUTexture* swapchainTex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, gpu.window, &swapchainTex,
                                               &sw, &sh) ||
        !swapchainTex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return nullptr;
    }

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture                = swapchainTex;
    colorTarget.clear_color            = {cfg.bgR, cfg.bgG, cfg.bgB, 1.0f};
    colorTarget.load_op                = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo dsTarget = {};
    dsTarget.texture                       = gpu.depthTexture;
    dsTarget.clear_depth                   = 1.0f;
    dsTarget.load_op                       = SDL_GPU_LOADOP_CLEAR;
    dsTarget.store_op                      = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &dsTarget);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(cmd);
    }
    return pass;
}

}  // namespace sdl3cpp::services::impl
