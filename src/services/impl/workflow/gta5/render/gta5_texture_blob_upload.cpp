#include "services/interfaces/workflow/gta5/render/gta5_texture_upload.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

Gta5GpuTexture UploadGta5TextureBlob(const Gta5TextureBlob& blob,
                                     SDL_GPUDevice* device,
                                     Gta5UploadBatch& uploads) {
    Gta5GpuTexture out;
    if (blob.bytes.empty() || !device ||
        !SDL_GPUTextureSupportsFormat(device, blob.format.gpu,
                                      SDL_GPU_TEXTURETYPE_2D,
                                      SDL_GPU_TEXTUREUSAGE_SAMPLER)) {
        return out;
    }
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = blob.format.gpu;
    info.width = blob.width;
    info.height = blob.height;
    info.layer_count_or_depth = 1;
    info.num_levels = blob.levels;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);
    if (!texture) return out;
    // Each mip staged into the frame's upload batch: the mips follow one
    // another in the blob, as they did in the dictionary.
    std::uint64_t offset = 0;
    for (std::uint32_t level = 0; level < blob.levels; ++level) {
        const std::uint32_t w = std::max(1u, blob.width >> level);
        const std::uint32_t h = std::max(1u, blob.height >> level);
        const std::uint64_t bytes = Gta5MipBytes(blob.format, w, h);
        if (!uploads.StageTexture(device, blob.bytes.data() + offset,
                                  static_cast<std::uint32_t>(bytes), texture,
                                  level, w, h)) {
            SDL_ReleaseGPUTexture(device, texture);
            return out;
        }
        offset += bytes;
    }
    out.texture = texture;
    out.levels = blob.levels;
    return out;
}

}  // namespace sdl3cpp::services::impl
