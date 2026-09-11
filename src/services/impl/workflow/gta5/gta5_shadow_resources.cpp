#include "services/interfaces/workflow/gta5/gta5_shadow_step.hpp"

namespace sdl3cpp::services::impl {

bool WorkflowGta5ShadowDrawStep::Ensure(SDL_GPUDevice* device, int size) {
    if (depth_ && size == size_) return sampler_ != nullptr;
    if (depth_) SDL_ReleaseGPUTexture(device, depth_);
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
                 SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width = static_cast<Uint32>(size);
    info.height = static_cast<Uint32>(size);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    depth_ = SDL_CreateGPUTexture(device, &info);
    size_ = size;
    if (!sampler_) {
        // Compared, not read: each tap answers "lit or not", and the
        // hardware filters the answers across the texel.
        SDL_GPUSamplerCreateInfo sampling = {};
        sampling.min_filter = SDL_GPU_FILTER_LINEAR;
        sampling.mag_filter = SDL_GPU_FILTER_LINEAR;
        sampling.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampling.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampling.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampling.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sampling.enable_compare = true;
        sampling.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        sampler_ = SDL_CreateGPUSampler(device, &sampling);
    }
    if ((!depth_ || !sampler_) && logger_) {
        logger_->Warn(std::string("gta5.shadow.draw: no shadow map: ") +
                      SDL_GetError());
    }
    return depth_ && sampler_;
}

}  // namespace sdl3cpp::services::impl
