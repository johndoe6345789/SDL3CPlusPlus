#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// How a G9 texture format byte maps onto the GPU. Block-compressed
/// formats upload exactly as stored; nothing is decoded on the way.
struct Gta5TextureFormat {
    SDL_GPUTextureFormat gpu{SDL_GPU_TEXTUREFORMAT_INVALID};
    /// Bytes per 4x4 block when compressed, per pixel when not.
    std::uint32_t bytes{0};
    bool compressed{true};
};

/// The G9 format at texture +0x1F. The sRGB variants map to UNORM on
/// purpose: the PNG path this replaces uploaded plain RGBA8, and the
/// composite's gamma was tuned against those values.
Gta5TextureFormat Gta5TextureFormatFor(std::uint8_t g9);

/// Bytes of one mip level.
std::uint64_t Gta5MipBytes(const Gta5TextureFormat& format,
                           std::uint32_t width, std::uint32_t height);

struct Gta5GpuTexture {
    SDL_GPUTexture* texture{nullptr};
    std::uint32_t levels{0};
};

/// The texture named `nameHash` in a loaded .ytd, on the GPU as stored,
/// mip chain included.
///
/// A .ytd holds name hashes at +0x20 (count at +0x28) and textures at
/// +0x30; a texture its size (+0x18, +0x1A), format (+0x1F), mip count
/// (+0x22) and pixels (+0x38). Mips follow mip 0 back to back -- checked
/// by decoding mip 1 where mip 0 ends against mip 0 halved. Empty when
/// missing, or in a format the device cannot sample.
Gta5GpuTexture UploadGta5DictionaryTexture(const Gta5Resource& ytd,
                                           std::uint32_t nameHash,
                                           SDL_GPUDevice* device);

/// Copy `levels` packed mips into `texture` with one command buffer.
bool CopyGta5Mips(SDL_GPUDevice* device, SDL_GPUTexture* texture,
                  const std::uint8_t* pixels, std::uint64_t total,
                  const Gta5TextureFormat& format, std::uint32_t width,
                  std::uint32_t height, std::uint32_t levels);

}  // namespace sdl3cpp::services::impl
