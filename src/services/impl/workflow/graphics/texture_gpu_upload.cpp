#include "services/interfaces/workflow/graphics/texture_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_gpu_transfer.hpp"

#include <stb_image.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

/// floor(log2(max(w,h))) + 1, i.e. how many mip levels a full chain needs.
Uint32 ComputeMipLevels(int width, int height) {
    int maxDim       = std::max(width, height);
    Uint32 numLevels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        numLevels++;
    }
    return numLevels;
}

SDL_GPUTexture* CreateUploadTexture(SDL_GPUDevice* device,
                                    const LoadedTextureImage& image,
                                    Uint32 numLevels) {
    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type                     = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tex_info.width                    = static_cast<Uint32>(image.width);
    tex_info.height                   = static_cast<Uint32>(image.height);
    tex_info.layer_count_or_depth     = 1;
    tex_info.num_levels               = numLevels;
    tex_info.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER |
                     (numLevels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);
    return SDL_CreateGPUTexture(device, &tex_info);
}

}  // namespace

UploadedTexture UploadTextureImage(SDL_GPUDevice* device,
                                   LoadedTextureImage& image) {
    const Uint32 numLevels = ComputeMipLevels(image.width, image.height);

    SDL_GPUTexture* texture = CreateUploadTexture(device, image, numLevels);
    if (!texture) {
        stbi_image_free(image.pixels);
        image.pixels = nullptr;
        throw std::runtime_error("texture.load: SDL_CreateGPUTexture failed: " +
                                 std::string(SDL_GetError()));
    }

    UploadPixelsToTexture(device, texture, image, numLevels);

    return UploadedTexture{texture, numLevels};
}

}  // namespace sdl3cpp::services::impl
