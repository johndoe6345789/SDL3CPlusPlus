#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"

namespace sdl3cpp::services::impl {

Gta5GpuTexture UploadGta5TextureBlob(const Gta5TextureBlob& blob,
                                     SDL_GPUDevice* device) {
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
    if (!CopyGta5Mips(device, texture, blob.bytes.data(), blob.bytes.size(),
                      blob.format, blob.width, blob.height, blob.levels)) {
        SDL_ReleaseGPUTexture(device, texture);
        return out;
    }
    out.texture = texture;
    out.levels = blob.levels;
    return out;
}

}  // namespace sdl3cpp::services::impl
