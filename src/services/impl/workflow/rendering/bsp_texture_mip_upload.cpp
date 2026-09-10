#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload_internal.hpp"

#include <algorithm>
#include <cstring>

namespace sdl3cpp::services::impl::bsp_texture_detail {

BspTextureUpload UploadRgba8WithMips(SDL_GPUDevice* device,
                                     const unsigned char* pixels, int width,
                                     int height) {
    int maxDim       = std::max(width, height);
    Uint32 numLevels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        ++numLevels;
    }

    SDL_GPUTextureCreateInfo ti = {};
    ti.type                     = SDL_GPU_TEXTURETYPE_2D;
    ti.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width                    = static_cast<Uint32>(width);
    ti.height                   = static_cast<Uint32>(height);
    ti.layer_count_or_depth     = 1;
    ti.num_levels               = numLevels;
    ti.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER |
               (numLevels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);
    auto* texture = SDL_CreateGPUTexture(device, &ti);

    const Uint32 dataSize = static_cast<Uint32>(width * height * 4);
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = dataSize;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbi);
    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        std::memcpy(mapped, pixels, dataSize);
        SDL_UnmapGPUTransferBuffer(device, staging);
    }

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer            = staging;

        SDL_GPUTextureRegion dst = {};
        dst.texture              = texture;
        dst.w                    = static_cast<Uint32>(width);
        dst.h                    = static_cast<Uint32>(height);
        dst.d                    = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        if (numLevels > 1) {
            SDL_GenerateMipmapsForGPUTexture(cmd, texture);
        }
    }
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staging);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_LINEAR;
    si.mag_filter               = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.enable_anisotropy        = true;
    si.max_anisotropy           = 16.0f;
    si.mip_lod_bias             = 0.5f;
    si.min_lod                  = 0.0f;
    si.max_lod                  = static_cast<float>(numLevels);

    return {texture, SDL_CreateGPUSampler(device, &si), false};
}

}  // namespace sdl3cpp::services::impl::bsp_texture_detail
