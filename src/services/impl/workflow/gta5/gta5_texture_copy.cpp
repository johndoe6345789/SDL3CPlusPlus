#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"

#include <algorithm>
#include <cstring>

namespace sdl3cpp::services::impl {

bool CopyGta5Mips(SDL_GPUDevice* device, SDL_GPUTexture* texture,
                  const std::uint8_t* pixels, std::uint64_t total,
                  const Gta5TextureFormat& format, std::uint32_t width,
                  std::uint32_t height, std::uint32_t levels) {
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = static_cast<Uint32>(total);
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &info);
    if (!transfer) return false;
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }
    std::memcpy(mapped, pixels, static_cast<std::size_t>(total));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
    std::uint64_t offset = 0;
    for (std::uint32_t level = 0; level < levels; ++level) {
        const std::uint32_t w = std::max(1u, width >> level);
        const std::uint32_t h = std::max(1u, height >> level);
        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = transfer;
        src.offset = static_cast<Uint32>(offset);  // rows tightly packed
        SDL_GPUTextureRegion dst{};
        dst.texture = texture;
        dst.mip_level = level;
        dst.w = w;
        dst.h = h;
        dst.d = 1;
        SDL_UploadToGPUTexture(pass, &src, &dst, false);
        offset += Gta5MipBytes(format, w, h);
    }
    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return true;
}

}  // namespace sdl3cpp::services::impl
