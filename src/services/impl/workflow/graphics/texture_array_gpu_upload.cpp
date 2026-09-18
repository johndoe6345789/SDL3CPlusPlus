#include "services/interfaces/workflow/graphics/texture_array_gpu_upload.hpp"

#include "services/interfaces/workflow/graphics/texture_array_gpu_parts.hpp"

#include <algorithm>
#include <stdexcept>

namespace sdl3cpp::services::impl {

UploadedTexture UploadTextureArrayBlocks(SDL_GPUDevice* device,
                                         const TextureArrayBlocksView& view) {
    SDL_GPUTexture* texture = CreateTextureArray(device, view);
    SDL_GPUTransferBuffer* transfer = StageTextureArray(device, view);
    if (!transfer) {
        SDL_ReleaseGPUTexture(device, texture);
        throw std::runtime_error("texture array: staging failed");
    }
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(commands);
    std::size_t offset = 0;
    for (std::uint32_t layer = 0; layer < view.layers; ++layer) {
        for (std::uint32_t mip = 0; mip < view.mips; ++mip) {
            SDL_GPUTextureTransferInfo source{};
            source.transfer_buffer = transfer;
            source.offset = static_cast<Uint32>(offset);
            SDL_GPUTextureRegion target{};
            target.texture = texture;
            target.mip_level = mip;
            target.layer = layer;
            target.w = std::max(view.width >> mip, 1u);
            target.h = std::max(view.height >> mip, 1u);
            target.d = 1;
            SDL_UploadToGPUTexture(pass, &source, &target, false);
            offset += TextureArrayMipBytes(view, mip);
        }
    }
    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(commands);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return {texture, view.mips};
}

}  // namespace sdl3cpp::services::impl
