#include "services/interfaces/workflow/bl4/bl4_instance_batch.hpp"

#include <algorithm>
#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint32_t kMatrixBytes = sizeof(float) * 16;

bool Grow(SDL_GPUDevice* device, Bl4InstanceBatch& batch, std::uint32_t needed) {
    std::uint32_t capacity = std::max(4096u, batch.capacity);
    while (capacity < needed) capacity *= 2;
    // Released buffers are kept until the GPU is done with them.
    if (batch.buffer) SDL_ReleaseGPUBuffer(device, batch.buffer);
    if (batch.transfer) SDL_ReleaseGPUTransferBuffer(device, batch.transfer);
    SDL_GPUBufferCreateInfo buffer = {};
    buffer.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    buffer.size = capacity * kMatrixBytes;
    batch.buffer = SDL_CreateGPUBuffer(device, &buffer);
    SDL_GPUTransferBufferCreateInfo transfer = {};
    transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer.size = capacity * kMatrixBytes;
    batch.transfer = SDL_CreateGPUTransferBuffer(device, &transfer);
    batch.capacity = (batch.buffer && batch.transfer) ? capacity : 0;
    return batch.capacity != 0;
}

}  // namespace

bool UploadBl4InstanceBatch(SDL_GPUDevice* device, Bl4InstanceBatch& batch) {
    const auto count = static_cast<std::uint32_t>(batch.matrices.size());
    if (!device || count == 0) return false;
    if (count > batch.capacity && !Grow(device, batch, count)) return false;
    const std::uint32_t bytes = count * kMatrixBytes;
    // Cycled: last frame's copy may still be in flight.
    void* mapped = SDL_MapGPUTransferBuffer(device, batch.transfer, true);
    if (!mapped) return false;
    std::memcpy(mapped, batch.matrices.data(), bytes);
    SDL_UnmapGPUTransferBuffer(device, batch.transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return false;
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    const SDL_GPUTransferBufferLocation source = {batch.transfer, 0};
    const SDL_GPUBufferRegion target = {batch.buffer, 0, bytes};
    SDL_UploadToGPUBuffer(copy, &source, &target, true);
    SDL_EndGPUCopyPass(copy);
    return SDL_SubmitGPUCommandBuffer(cmd);
}

void ReleaseBl4InstanceBatch(SDL_GPUDevice* device, Bl4InstanceBatch& batch) {
    if (device) {
        if (batch.buffer) SDL_ReleaseGPUBuffer(device, batch.buffer);
        if (batch.transfer) SDL_ReleaseGPUTransferBuffer(device, batch.transfer);
    }
    batch = Bl4InstanceBatch{};
}

}  // namespace sdl3cpp::services::impl
