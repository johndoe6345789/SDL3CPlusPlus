#include "services/interfaces/workflow/graphics/gpu_readback_blit.hpp"

#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

BlittedSwapchainStaging BlitSwapchainToStaging(SDL_GPUDevice* device,
                                               SDL_Window* window) {
    BlittedSwapchainStaging out;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return out;

    // We need a swapchain texture to blit from (GPU-only surface)
    SDL_GPUTexture* swapchain_tex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain_tex, &sw,
                                               &sh) ||
        !swapchain_tex) {
        SDL_CancelGPUCommandBuffer(cmd);
        return out;
    }

    SDL_GPUTextureFormat format =
        SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type                     = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format                   = format;
    tex_info.width                    = sw;
    tex_info.height                   = sh;
    tex_info.layer_count_or_depth     = 1;
    tex_info.num_levels               = 1;
    tex_info.usage =
        SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

    SDL_GPUTexture* staging_tex = SDL_CreateGPUTexture(device, &tex_info);
    if (!staging_tex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return out;
    }

    SDL_GPUBlitInfo blit     = {};
    blit.source.texture      = swapchain_tex;
    blit.source.w            = sw;
    blit.source.h            = sh;
    blit.destination.texture = staging_tex;
    blit.destination.w       = sw;
    blit.destination.h       = sh;
    blit.load_op             = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter              = SDL_GPU_FILTER_LINEAR;

    SDL_BlitGPUTexture(cmd, &blit);
    SDL_SubmitGPUCommandBuffer(cmd);

    out.texture = staging_tex;
    out.width   = sw;
    out.height  = sh;
    return out;
}

}  // namespace sdl3cpp::services::impl
