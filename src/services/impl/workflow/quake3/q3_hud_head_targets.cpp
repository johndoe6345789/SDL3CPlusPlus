#include "services/interfaces/workflow/quake3/q3_hud_head_targets.hpp"

namespace sdl3cpp::services::impl {

rendering::FragmentUniformData DefaultHeadPortraitLighting() {
    rendering::FragmentUniformData fu{};
    fu.material[0] = 0.7f;  // roughness
    fu.material[1] = 0.0f;  // metallic
    // Soft front-right key light
    fu.light_dir[0]   = 0.5f;
    fu.light_dir[1]   = -0.7f;
    fu.light_dir[2]   = -0.5f;
    fu.light_color[0] = 1.2f;
    fu.light_color[1] = 1.1f;
    fu.light_color[2] = 1.0f;
    fu.ambient[0] = fu.ambient[1] = fu.ambient[2] = 0.35f;
    return fu;
}

HeadRenderTargets CreateHeadRenderTargets(SDL_GPUDevice* device,
                                          SDL_Window* window, int size) {
    HeadRenderTargets out;

    SDL_GPUTextureCreateInfo ci{};
    ci.type                 = SDL_GPU_TEXTURETYPE_2D;
    ci.width                = size;
    ci.height               = size;
    ci.layer_count_or_depth = 1;
    ci.num_levels           = 1;

    // Must match the swapchain format so gpu_pipeline_textured (compiled
    // for the swapchain) can render into this target without a
    // format-mismatch error.
    const SDL_GPUTextureFormat scFmt =
        window ? SDL_GetGPUSwapchainTextureFormat(device, window)
               : SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;

    ci.format = scFmt;
    ci.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    out.color = SDL_CreateGPUTexture(device, &ci);

    ci.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    ci.usage  = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    out.depth = SDL_CreateGPUTexture(device, &ci);

    out.ready = out.color && out.depth;
    return out;
}

}  // namespace sdl3cpp::services::impl
