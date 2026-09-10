#include "services/interfaces/workflow/graphics/texture_gpu_transfer.hpp"

#include <stb_image.h>

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

void UploadPixelsToTexture(SDL_GPUDevice* device, SDL_GPUTexture* texture,
                           LoadedTextureImage& image, Uint32 numLevels) {
    const Uint32 data_size =
        static_cast<Uint32>(image.width * image.height * 4);

    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size  = data_size;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tbuf_info);
    if (!transfer) {
        stbi_image_free(image.pixels);
        image.pixels = nullptr;
        SDL_ReleaseGPUTexture(device, texture);
        throw std::runtime_error(
            "texture.load: Failed to create transfer buffer");
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(mapped, image.pixels, data_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    stbi_image_free(image.pixels);
    image.pixels = nullptr;

    SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer            = transfer;
    src.offset                     = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture              = texture;
    dst.w                    = static_cast<Uint32>(image.width);
    dst.h                    = static_cast<Uint32>(image.height);
    dst.d                    = 1;

    SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);

    // Generate mipmaps from the uploaded base level.
    if (numLevels > 1) {
        SDL_GenerateMipmapsForGPUTexture(cmd, texture);
    }

    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
}

}  // namespace sdl3cpp::services::impl
