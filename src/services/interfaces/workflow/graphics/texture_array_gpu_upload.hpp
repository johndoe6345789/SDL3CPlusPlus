#pragma once

#include "services/interfaces/workflow/graphics/texture_gpu_upload.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstddef>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// A block-compressed texture array ready for the GPU as-is: `layers`
/// layers, each `mips` levels, stored layer-major and tightly packed
/// (the order a DX10 DDS array already uses). A plain 2D texture is the
/// one-layer case with `type` SDL_GPU_TEXTURETYPE_2D.
struct TextureArrayBlocksView {
    SDL_GPUTextureType type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
    SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_INVALID;
    std::uint32_t width = 0, height = 0, layers = 0, mips = 1;
    std::uint32_t blockBytes = 8;  ///< per 4x4 block
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
};

/// Creates a sampler texture of `view.type` and uploads
/// every layer's every mip in one copy pass, compressed blocks passed
/// straight through. Throws std::runtime_error when the device cannot
/// sample the format or a GPU resource cannot be created, releasing
/// anything already made.
UploadedTexture UploadTextureArrayBlocks(SDL_GPUDevice* device,
                                         const TextureArrayBlocksView& view);

}  // namespace sdl3cpp::services::impl
