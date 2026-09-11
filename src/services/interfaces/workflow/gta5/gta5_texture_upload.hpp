#pragma once

#include "services/interfaces/workflow/gta5/gta5_upload_batch.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

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
/// purpose: the composite's gamma was tuned against plain RGBA8 values.
Gta5TextureFormat Gta5TextureFormatFor(std::uint8_t g9);

/// Bytes of one mip level.
std::uint64_t Gta5MipBytes(const Gta5TextureFormat& format,
                           std::uint32_t width, std::uint32_t height);

/// One texture's pixels, read out of its dictionary and ready to upload.
/// Empty `bytes` means there is no such texture, or its format is not
/// handled -- which still settles the question for anyone waiting on it.
struct Gta5TextureBlob {
    std::uint32_t hash{0};
    Gta5TextureFormat format;
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t levels{0};
    std::vector<std::uint8_t> bytes;  // mips back to back
};

/// Read the texture named `nameHash` from a loaded .ytd. It touches only
/// `ytd`, so worker threads call it. A .ytd holds name hashes at +0x20
/// (count at +0x28) and textures at +0x30; a texture its size (+0x18,
/// +0x1A), format (+0x1F), mip count (+0x22) and pixels (+0x38). Mips
/// follow mip 0 back to back.
/// `dictionary` is where the dictionary starts: 0 in a .ytd, whose root
/// it is; a drawable's shader group points at its own embedded one.
Gta5TextureBlob ReadGta5DictionaryTexture(const Gta5Resource& ytd,
                                          std::uint32_t nameHash,
                                          std::int64_t dictionary = 0);

struct Gta5GpuTexture {
    SDL_GPUTexture* texture{nullptr};
    std::uint32_t levels{0};
};

/// Create a blob's texture and stage its mips in `uploads`, which the
/// frame submits. Main thread only. Empty when the blob is, or the
/// device cannot sample its format.
Gta5GpuTexture UploadGta5TextureBlob(const Gta5TextureBlob& blob,
                                     SDL_GPUDevice* device,
                                     Gta5UploadBatch& uploads);

}  // namespace sdl3cpp::services::impl
