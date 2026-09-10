#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload.hpp"

namespace sdl3cpp::services::impl {

BspTextureUpload CreateBspWhiteTexture(SDL_GPUDevice* device) {
    SDL_GPUTextureCreateInfo ti = {};
    ti.type                     = SDL_GPU_TEXTURETYPE_2D;
    ti.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width                    = 1;
    ti.height                   = 1;
    ti.layer_count_or_depth     = 1;
    ti.num_levels               = 1;
    ti.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    auto* texture               = SDL_CreateGPUTexture(device, &ti);

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = 4;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbi);
    if (auto* mapped = static_cast<uint8_t*>(
            SDL_MapGPUTransferBuffer(device, staging, false))) {
        mapped[0] = mapped[1] = mapped[2] = mapped[3] = 255;
        SDL_UnmapGPUTransferBuffer(device, staging);
    }

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer            = staging;
        SDL_GPUTextureRegion dst       = {};
        dst.texture                    = texture;
        dst.w                          = 1;
        dst.h                          = 1;
        dst.d                          = 1;
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staging);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_LINEAR;
    si.mag_filter               = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    return {texture, SDL_CreateGPUSampler(device, &si), false};
}

}  // namespace sdl3cpp::services::impl
