#include "services/interfaces/workflow/graphics/texture_array_gpu_parts.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::size_t TextureArrayMipBytes(const TextureArrayBlocksView& view,
                                 std::uint32_t mip) {
    const std::uint32_t w = std::max(view.width >> mip, 1u);
    const std::uint32_t h = std::max(view.height >> mip, 1u);
    return static_cast<std::size_t>((w + 3) / 4) * ((h + 3) / 4) *
           view.blockBytes;
}

SDL_GPUTexture* CreateTextureArray(SDL_GPUDevice* device,
                                   const TextureArrayBlocksView& view) {
    if (!SDL_GPUTextureSupportsFormat(device, view.format,
                                      view.type,
                                      SDL_GPU_TEXTUREUSAGE_SAMPLER)) {
        throw std::runtime_error("texture array: format not supported");
    }
    SDL_GPUTextureCreateInfo info{};
    info.type = view.type;
    info.format = view.format;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = view.width;
    info.height = view.height;
    info.layer_count_or_depth = view.layers;
    info.num_levels = view.mips;
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);
    if (!texture) throw std::runtime_error("texture array: create failed");
    return texture;
}

SDL_GPUTransferBuffer* StageTextureArray(SDL_GPUDevice* device,
                                         const TextureArrayBlocksView& view) {
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = static_cast<Uint32>(view.size);
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &info);
    void* mapped =
        transfer ? SDL_MapGPUTransferBuffer(device, transfer, false) : nullptr;
    if (!mapped) {
        if (transfer) SDL_ReleaseGPUTransferBuffer(device, transfer);
        return nullptr;
    }
    std::memcpy(mapped, view.data, view.size);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    return transfer;
}

}  // namespace sdl3cpp::services::impl
