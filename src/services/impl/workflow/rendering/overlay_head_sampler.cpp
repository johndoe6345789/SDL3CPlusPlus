#include "services/interfaces/workflow/rendering/overlay_head_sampler.hpp"

namespace sdl3cpp::services::impl {

bool EnsureHeadSampler(OverlaySwEndResources& res) {
    if (res.headSampler) {
        return true;
    }
    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter               = SDL_GPU_FILTER_LINEAR;
    sci.mag_filter               = SDL_GPU_FILTER_LINEAR;
    sci.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sci.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    res.headSampler              = SDL_CreateGPUSampler(res.device, &sci);
    return res.headSampler != nullptr;
}

}  // namespace sdl3cpp::services::impl
