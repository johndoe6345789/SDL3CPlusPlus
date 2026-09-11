#include "services/interfaces/workflow/gta5/gta5_upload_batch.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

// Room for the largest texture GTA V ships -- 4096 x 4096 BC7 with its
// mips, about 22 MB -- and plenty of meshes besides.
constexpr std::uint32_t kCapacity = 64u << 20;
// A whole compressed block, and more than Vulkan asks of a copy offset.
constexpr std::uint32_t kAlign = 16;

}  // namespace

std::uint8_t* Gta5UploadBatch::Reserve(SDL_GPUDevice* device,
                                       std::uint32_t size,
                                       std::uint32_t& at) {
    if (!device || size == 0 || size > kCapacity) return nullptr;
    std::uint32_t start = (used_ + kAlign - 1) / kAlign * kAlign;
    if (std::uint64_t{start} + size > kCapacity) {
        Flush(device);
        start = 0;
    }
    if (!transfer_) {
        SDL_GPUTransferBufferCreateInfo info = {};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = kCapacity;
        transfer_ = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transfer_) return nullptr;
    }
    if (!mapped_) {
        // Cycled: last frame's copy may still be reading it.
        mapped_ = static_cast<std::uint8_t*>(
            SDL_MapGPUTransferBuffer(device, transfer_, true));
        if (!mapped_) return nullptr;
    }
    used_ = start + size;
    at = start;
    return mapped_ + start;
}

bool Gta5UploadBatch::StageBuffer(SDL_GPUDevice* device, const void* data,
                                  std::uint32_t size, SDL_GPUBuffer* target,
                                  std::uint32_t offset) {
    std::uint32_t at = 0;
    std::uint8_t* into = Reserve(device, size, at);
    if (!into || !target) return false;
    std::memcpy(into, data, size);
    copies_.push_back({target, nullptr, offset, 0, 0, at, size});
    return true;
}

bool Gta5UploadBatch::StageTexture(SDL_GPUDevice* device, const void* data,
                                   std::uint32_t size,
                                   SDL_GPUTexture* texture,
                                   std::uint32_t level, std::uint32_t width,
                                   std::uint32_t height) {
    std::uint32_t at = 0;
    std::uint8_t* into = Reserve(device, size, at);
    if (!into || !texture) return false;
    std::memcpy(into, data, size);
    copies_.push_back({nullptr, texture, level, width, height, at, size});
    return true;
}

}  // namespace sdl3cpp::services::impl
