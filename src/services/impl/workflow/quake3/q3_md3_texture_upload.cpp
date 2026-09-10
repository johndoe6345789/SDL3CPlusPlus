#include "services/interfaces/workflow/quake3/q3_md3_gpu_upload.hpp"

#include <cstring>

namespace sdl3cpp::q3 {

SDL_GPUTexture* UploadMd3Texture(SDL_GPUDevice* device, const uint8_t* pixels,
                                 int width, int height) {
    SDL_GPUTextureCreateInfo ti = {};
    ti.type                     = SDL_GPU_TEXTURETYPE_2D;
    ti.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width                    = static_cast<uint32_t>(width);
    ti.height                   = static_cast<uint32_t>(height);
    ti.layer_count_or_depth     = 1;
    ti.num_levels               = 1;
    ti.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    auto* texture               = SDL_CreateGPUTexture(device, &ti);
    if (!texture) {
        return nullptr;
    }

    const uint32_t bytes = static_cast<uint32_t>(width * height * 4);
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = bytes;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbi);
    if (!staging) {
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }

    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        std::memcpy(mapped, pixels, bytes);
        SDL_UnmapGPUTransferBuffer(device, staging);
    }

    auto* cmd  = SDL_AcquireGPUCommandBuffer(device);
    auto* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer            = staging;
    src.pixels_per_row             = static_cast<uint32_t>(width);
    src.rows_per_layer             = static_cast<uint32_t>(height);

    SDL_GPUTextureRegion dst = {};
    dst.texture              = texture;
    dst.w                    = static_cast<uint32_t>(width);
    dst.h                    = static_cast<uint32_t>(height);
    dst.d                    = 1;

    SDL_UploadToGPUTexture(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staging);
    return texture;
}

}  // namespace sdl3cpp::q3
