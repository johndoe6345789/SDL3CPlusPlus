#include "services/interfaces/workflow/gta5/gta5_geometry_arena.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

bool Copy(SDL_GPUDevice* device, SDL_GPUBuffer* vertices,
          SDL_GPUBuffer* indices, const Gta5ArenaSlot& slot,
          const void* vertexData, const std::uint16_t* indexData) {
    const std::uint32_t vertexBytes =
        slot.vertexCount * Gta5GeometryArena::kVertexStride;
    const std::uint32_t indexBytes =
        slot.indexCount * static_cast<std::uint32_t>(sizeof(std::uint16_t));
    SDL_GPUTransferBufferCreateInfo info = {};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = vertexBytes + indexBytes;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &info);
    if (!transfer) return false;
    auto* mapped =
        static_cast<std::uint8_t*>(SDL_MapGPUTransferBuffer(device, transfer,
                                                            false));
    SDL_GPUCommandBuffer* cmd = mapped ? SDL_AcquireGPUCommandBuffer(device)
                                       : nullptr;
    if (!cmd) {
        if (mapped) SDL_UnmapGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }
    std::memcpy(mapped, vertexData, vertexBytes);
    std::memcpy(mapped + vertexBytes, indexData, indexBytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    const SDL_GPUTransferBufferLocation vertexSource = {transfer, 0};
    const SDL_GPUBufferRegion vertexTarget = {
        vertices, slot.vertexOffset * Gta5GeometryArena::kVertexStride,
        vertexBytes};
    SDL_UploadToGPUBuffer(copy, &vertexSource, &vertexTarget, false);
    const SDL_GPUTransferBufferLocation indexSource = {transfer, vertexBytes};
    const SDL_GPUBufferRegion indexTarget = {
        indices,
        slot.firstIndex * static_cast<std::uint32_t>(sizeof(std::uint16_t)),
        indexBytes};
    SDL_UploadToGPUBuffer(copy, &indexSource, &indexTarget, false);
    SDL_EndGPUCopyPass(copy);
    const bool submitted = SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return submitted;
}

}  // namespace

bool Gta5GeometryArena::Upload(SDL_GPUDevice* device, const void* vertices,
                               std::uint32_t vertexCount,
                               const std::uint16_t* indices,
                               std::uint32_t indexCount,
                               Gta5ArenaSlot& slot) {
    if (!device || vertexCount == 0 || indexCount == 0) return false;
    if (!Place(vertexCount, indexCount, slot) &&
        !(AddBlock(device) && Place(vertexCount, indexCount, slot))) {
        return false;
    }
    if (Copy(device, Vertices(slot.block), Indices(slot.block), slot,
             vertices, indices)) {
        return true;
    }
    Free(slot);
    slot = {};
    return false;
}

}  // namespace sdl3cpp::services::impl
