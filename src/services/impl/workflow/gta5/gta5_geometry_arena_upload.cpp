#include "services/interfaces/workflow/gta5/gta5_geometry_arena.hpp"

namespace sdl3cpp::services::impl {

bool Gta5GeometryArena::Upload(SDL_GPUDevice* device,
                               Gta5UploadBatch& uploads,
                               const void* vertices,
                               std::uint32_t vertexCount,
                               const std::uint16_t* indices,
                               std::uint32_t indexCount,
                               Gta5ArenaSlot& slot) {
    if (!device || vertexCount == 0 || indexCount == 0) return false;
    if (!Place(vertexCount, indexCount, slot) &&
        !(AddBlock(device) && Place(vertexCount, indexCount, slot))) {
        return false;
    }
    const auto indexBytes =
        indexCount * static_cast<std::uint32_t>(sizeof(std::uint16_t));
    if (uploads.StageBuffer(device, vertices, vertexCount * kVertexStride,
                            Vertices(slot.block),
                            slot.vertexOffset * kVertexStride) &&
        uploads.StageBuffer(
            device, indices, indexBytes, Indices(slot.block),
            slot.firstIndex *
                static_cast<std::uint32_t>(sizeof(std::uint16_t)))) {
        return true;
    }
    Free(slot);
    slot = {};
    return false;
}

}  // namespace sdl3cpp::services::impl
