#include "services/interfaces/workflow/rendering/bsp_portal_view_targets.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr uint32_t kPortalSize = 512;

}  // namespace

PortalViewTargets EnsurePortalViewTargets(SDL_GPUDevice* device,
                                          SDL_Window* window,
                                          WorkflowContext& context) {
    PortalViewTargets targets;
    targets.colorTex =
        context.Get<SDL_GPUTexture*>("bsp_portal_view_texture", nullptr);
    targets.depthTex =
        context.Get<SDL_GPUTexture*>("bsp_portal_view_depth", nullptr);
    targets.sampler =
        context.Get<SDL_GPUSampler*>("bsp_portal_view_sampler", nullptr);

    if (!targets.colorTex) {
        SDL_GPUTextureCreateInfo ti = {};
        ti.type                     = SDL_GPU_TEXTURETYPE_2D;
        ti.format = SDL_GetGPUSwapchainTextureFormat(device, window);
        ti.width  = kPortalSize;
        ti.height = kPortalSize;
        ti.layer_count_or_depth = 1;
        ti.num_levels           = 1;
        ti.usage =
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        targets.colorTex = SDL_CreateGPUTexture(device, &ti);
        if (targets.colorTex) {
            context.Set<SDL_GPUTexture*>("bsp_portal_view_texture",
                                         targets.colorTex);
        }
    }

    if (!targets.depthTex) {
        SDL_GPUTextureCreateInfo di = {};
        di.type                     = SDL_GPU_TEXTURETYPE_2D;
        di.format                   = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        di.width                    = kPortalSize;
        di.height                   = kPortalSize;
        di.layer_count_or_depth     = 1;
        di.num_levels               = 1;
        di.usage                    = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        targets.depthTex            = SDL_CreateGPUTexture(device, &di);
        if (targets.depthTex) {
            context.Set<SDL_GPUTexture*>("bsp_portal_view_depth",
                                         targets.depthTex);
        }
    }

    if (!targets.sampler) {
        SDL_GPUSamplerCreateInfo si = {};
        si.min_filter               = SDL_GPU_FILTER_LINEAR;
        si.mag_filter               = SDL_GPU_FILTER_LINEAR;
        si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        targets.sampler             = SDL_CreateGPUSampler(device, &si);
        if (targets.sampler) {
            context.Set<SDL_GPUSampler*>("bsp_portal_view_sampler",
                                         targets.sampler);
        }
    }
    return targets;
}

}  // namespace sdl3cpp::services::impl
