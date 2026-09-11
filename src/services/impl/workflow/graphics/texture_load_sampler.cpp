#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

SDL_GPUSampler* CreateTextureLoadSampler(SDL_GPUDevice* device,
                                         SDL_GPUTexture* texture,
                                         Uint32 numLevels,
                                         float mipLodBias) {
    SDL_GPUSamplerCreateInfo samp_info = {};
    samp_info.min_filter               = SDL_GPU_FILTER_LINEAR;
    samp_info.mag_filter               = SDL_GPU_FILTER_LINEAR;
    samp_info.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samp_info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.enable_anisotropy        = true;
    samp_info.max_anisotropy           = 16.0f;
    samp_info.mip_lod_bias = mipLodBias;
    samp_info.min_lod      = 0.0f;
    samp_info.max_lod      = static_cast<float>(numLevels);

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(device, &samp_info);
    if (!sampler) {
        SDL_ReleaseGPUTexture(device, texture);
        throw std::runtime_error("texture.load: SDL_CreateGPUSampler failed");
    }
    return sampler;
}

}  // namespace sdl3cpp::services::impl
