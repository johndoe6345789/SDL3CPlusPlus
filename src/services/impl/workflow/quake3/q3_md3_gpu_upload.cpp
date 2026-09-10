#include "services/interfaces/workflow/quake3/q3_md3_gpu_upload.hpp"
#include "services/interfaces/workflow/quake3/q3_pk3_reader.hpp"

#include <stb_image.h>

#include <cstring>

namespace sdl3cpp::q3 {

SDL_GPUBuffer* UploadMd3Buffer(SDL_GPUDevice* device,
                               SDL_GPUBufferUsageFlags usage, const void* data,
                               uint32_t size) {
    SDL_GPUBufferCreateInfo bi = {};
    bi.usage                   = usage;
    bi.size                    = size;
    auto* buffer               = SDL_CreateGPUBuffer(device, &bi);
    if (!buffer) {
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = size;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbi);
    if (!staging) {
        SDL_ReleaseGPUBuffer(device, buffer);
        return nullptr;
    }

    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        std::memcpy(mapped, data, size);
        SDL_UnmapGPUTransferBuffer(device, staging);
    }

    auto* cmd                         = SDL_AcquireGPUCommandBuffer(device);
    auto* copy                        = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src = {staging, 0};
    SDL_GPUBufferRegion dst           = {buffer, 0, size};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staging);
    return buffer;
}

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

SDL_GPUSampler* MakeMd3LinearSampler(SDL_GPUDevice* device) {
    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_LINEAR;
    si.mag_filter               = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    return SDL_CreateGPUSampler(device, &si);
}

SDL_GPUTexture* TryLoadMd3Texture(SDL_GPUDevice* device, const std::string& pk3,
                                  const std::vector<std::string>& candidates) {
    for (const auto& entry : candidates) {
        const auto raw = ReadPk3Entry(pk3, entry);
        if (raw.empty()) {
            continue;
        }
        int width = 0, height = 0, channels = 0;
        unsigned char* pixels =
            stbi_load_from_memory(raw.data(), static_cast<int>(raw.size()),
                                  &width, &height, &channels, 4);
        if (!pixels) {
            continue;
        }
        auto* texture = UploadMd3Texture(device, pixels, width, height);
        stbi_image_free(pixels);
        if (texture) {
            return texture;
        }
    }
    return nullptr;
}

}  // namespace sdl3cpp::q3
