#include "services/interfaces/workflow/graphics/texture_load_helpers.hpp"

#include <stb_image.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string ResolveTextureImagePath(const std::string& raw) {
    std::string resolved = raw;
    if (!resolved.empty() && resolved[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) resolved = std::string(home) + resolved.substr(1);
    }
    return resolved;
}

LoadedTextureImage LoadTextureImagePixels(const std::string& path) {
    LoadedTextureImage image;
    image.pixels =
        stbi_load(path.c_str(), &image.width, &image.height, nullptr, 4);
    if (!image.pixels) {
        throw std::runtime_error("texture.load: Failed to load image: " +
                                 path + " (" +
                                 std::string(stbi_failure_reason()) + ")");
    }
    return image;
}

void FreeTextureImagePixels(LoadedTextureImage& image) {
    if (image.pixels) {
        stbi_image_free(image.pixels);
        image.pixels = nullptr;
    }
}

UploadedTexture UploadTextureImage(SDL_GPUDevice* device,
                                   LoadedTextureImage& image) {
    // Calculate mip levels: floor(log2(max(w,h))) + 1.
    int maxDim = std::max(image.width, image.height);
    Uint32 numLevels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        numLevels++;
    }

    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tex_info.width = static_cast<Uint32>(image.width);
    tex_info.height = static_cast<Uint32>(image.height);
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = numLevels;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
                      (numLevels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &tex_info);
    if (!texture) {
        stbi_image_free(image.pixels);
        image.pixels = nullptr;
        throw std::runtime_error("texture.load: SDL_CreateGPUTexture failed: " +
                                 std::string(SDL_GetError()));
    }

    const Uint32 data_size =
        static_cast<Uint32>(image.width * image.height * 4);

    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size = data_size;
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

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUTextureRegion dst = {};
    dst.texture = texture;
    dst.w = static_cast<Uint32>(image.width);
    dst.h = static_cast<Uint32>(image.height);
    dst.d = 1;

    SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);

    // Generate mipmaps from the uploaded base level.
    if (numLevels > 1) {
        SDL_GenerateMipmapsForGPUTexture(cmd, texture);
    }

    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return UploadedTexture{texture, numLevels};
}

SDL_GPUSampler* CreateTextureLoadSampler(SDL_GPUDevice* device,
                                         SDL_GPUTexture* texture,
                                         Uint32 numLevels) {
    SDL_GPUSamplerCreateInfo samp_info = {};
    samp_info.min_filter = SDL_GPU_FILTER_LINEAR;
    samp_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    samp_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samp_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samp_info.enable_anisotropy = true;
    samp_info.max_anisotropy = 16.0f;
    samp_info.mip_lod_bias = 0.5f;  // bias toward higher mip = less aliasing.
    samp_info.min_lod = 0.0f;
    samp_info.max_lod = static_cast<float>(numLevels);

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(device, &samp_info);
    if (!sampler) {
        SDL_ReleaseGPUTexture(device, texture);
        throw std::runtime_error("texture.load: SDL_CreateGPUSampler failed");
    }
    return sampler;
}

}  // namespace sdl3cpp::services::impl
