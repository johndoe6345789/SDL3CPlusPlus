#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload.hpp"

#include <stb_image.h>
#include <zip.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// Uploads already-decoded RGBA8 pixels, generating a full mip chain.
BspTextureUpload UploadRgba8WithMips(SDL_GPUDevice* device,
                                     const unsigned char* pixels, int width,
                                     int height) {
    int maxDim       = std::max(width, height);
    Uint32 numLevels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        ++numLevels;
    }

    SDL_GPUTextureCreateInfo ti = {};
    ti.type                     = SDL_GPU_TEXTURETYPE_2D;
    ti.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width                    = static_cast<Uint32>(width);
    ti.height                   = static_cast<Uint32>(height);
    ti.layer_count_or_depth     = 1;
    ti.num_levels               = numLevels;
    ti.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER |
               (numLevels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);
    auto* texture = SDL_CreateGPUTexture(device, &ti);

    const Uint32 dataSize = static_cast<Uint32>(width * height * 4);
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = dataSize;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbi);
    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        std::memcpy(mapped, pixels, dataSize);
        SDL_UnmapGPUTransferBuffer(device, staging);
    }

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer            = staging;

        SDL_GPUTextureRegion dst = {};
        dst.texture              = texture;
        dst.w                    = static_cast<Uint32>(width);
        dst.h                    = static_cast<Uint32>(height);
        dst.d                    = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        if (numLevels > 1) {
            SDL_GenerateMipmapsForGPUTexture(cmd, texture);
        }
    }
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staging);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_LINEAR;
    si.mag_filter               = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.enable_anisotropy        = true;
    si.max_anisotropy           = 16.0f;
    si.mip_lod_bias             = 0.5f;
    si.min_lod                  = 0.0f;
    si.max_lod                  = static_cast<float>(numLevels);

    return {texture, SDL_CreateGPUSampler(device, &si), false};
}

/// Tries one pk3 entry; returns a null-texture upload if it doesn't decode.
BspTextureUpload TryLoadEntry(zip_t* archive, SDL_GPUDevice* device,
                              const std::string& entryName) {
    zip_stat_t stat;
    if (zip_stat(archive, entryName.c_str(), 0, &stat) != 0) {
        return {};
    }
    zip_file_t* file = zip_fopen(archive, entryName.c_str(), 0);
    if (!file) {
        return {};
    }
    std::vector<uint8_t> data(stat.size);
    zip_fread(file, data.data(), stat.size);
    zip_fclose(file);

    int width = 0, height = 0, channels = 0;
    unsigned char* pixels =
        stbi_load_from_memory(data.data(), static_cast<int>(data.size()),
                              &width, &height, &channels, 4);
    if (!pixels) {
        return {};
    }
    BspTextureUpload upload =
        UploadRgba8WithMips(device, pixels, width, height);
    stbi_image_free(pixels);
    return upload;
}

}  // namespace

BspTextureUpload LoadBspTextureFromPk3(
    zip_t* archive, SDL_GPUDevice* device, const std::string& texName,
    const std::map<std::string, std::string>& shaderImages) {
    static const char* kExtensions[] = {".jpg", ".tga", ".png"};

    std::vector<std::string> bases{texName};
    const auto viaScript = shaderImages.find(texName);
    if (viaScript != shaderImages.end() && viaScript->second != texName) {
        bases.push_back(viaScript->second);
    }

    for (const std::string& base : bases) {
        for (const char* ext : kExtensions) {
            BspTextureUpload upload = TryLoadEntry(archive, device, base + ext);
            if (upload.texture) {
                upload.viaShader = (base != texName);
                return upload;
            }
        }
    }
    return {};
}

BspTextureUpload CreateBspWhiteTexture(SDL_GPUDevice* device) {
    SDL_GPUTextureCreateInfo ti = {};
    ti.type                     = SDL_GPU_TEXTURETYPE_2D;
    ti.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width                    = 1;
    ti.height                   = 1;
    ti.layer_count_or_depth     = 1;
    ti.num_levels               = 1;
    ti.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    auto* texture               = SDL_CreateGPUTexture(device, &ti);

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = 4;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbi);
    if (auto* mapped = static_cast<uint8_t*>(
            SDL_MapGPUTransferBuffer(device, staging, false))) {
        mapped[0] = mapped[1] = mapped[2] = mapped[3] = 255;
        SDL_UnmapGPUTransferBuffer(device, staging);
    }

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer            = staging;
        SDL_GPUTextureRegion dst       = {};
        dst.texture                    = texture;
        dst.w                          = 1;
        dst.h                          = 1;
        dst.d                          = 1;
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staging);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_LINEAR;
    si.mag_filter               = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    return {texture, SDL_CreateGPUSampler(device, &si), false};
}

}  // namespace sdl3cpp::services::impl
