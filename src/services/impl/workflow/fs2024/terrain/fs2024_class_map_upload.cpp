#include "services/interfaces/workflow/fs2024/terrain/fs2024_class_map_upload.hpp"

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUSampler* NearestClamped(SDL_GPUDevice* device) {
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = SDL_GPU_FILTER_NEAREST;
    info.mag_filter = SDL_GPU_FILTER_NEAREST;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler(device, &info);
}

}  // namespace

Fs2024ClassMapGpu UploadFs2024ClassMap(SDL_GPUDevice* device,
                                       const std::vector<std::uint8_t>& classes,
                                       int size) {
    SDL_GPUTextureCreateInfo textureInfo{};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    textureInfo.width = textureInfo.height = static_cast<Uint32>(size);
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    Fs2024ClassMapGpu map;
    map.texture = SDL_CreateGPUTexture(device, &textureInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(classes.size());
    SDL_GPUTransferBuffer* transfer =
        map.texture ? SDL_CreateGPUTransferBuffer(device, &transferInfo)
                    : nullptr;
    void* mapped =
        transfer ? SDL_MapGPUTransferBuffer(device, transfer, false) : nullptr;
    if (!mapped) {
        if (transfer) SDL_ReleaseGPUTransferBuffer(device, transfer);
        if (map.texture) SDL_ReleaseGPUTexture(device, map.texture);
        throw std::runtime_error("fs2024 class map: upload failed");
    }
    std::memcpy(mapped, classes.data(), classes.size());
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer = transfer;
    SDL_GPUTextureRegion target{};
    target.texture = map.texture;
    target.w = target.h = static_cast<Uint32>(size);
    target.d = 1;
    SDL_UploadToGPUTexture(pass, &source, &target, false);
    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(commands);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    map.sampler = NearestClamped(device);
    return map;
}

}  // namespace sdl3cpp::services::impl
