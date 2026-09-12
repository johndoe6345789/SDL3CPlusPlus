#include "services/interfaces/workflow/gta5/stream/gta5_geometry_arena.hpp"

namespace sdl3cpp::services::impl {
namespace {

// 2M vertices (80 MB) and 8M indices (16 MB) a block: a district fits in
// one or two, and the largest submesh, 65,536 vertices, fits in any.
constexpr std::uint32_t kBlockVertices = 1u << 21;
constexpr std::uint32_t kBlockIndices = 1u << 23;

}  // namespace

bool Gta5GeometryArena::Place(std::uint32_t vertexCount,
                              std::uint32_t indexCount, Gta5ArenaSlot& slot) {
    for (std::size_t b = 0; b < blocks_.size(); ++b) {
        Block& block = blocks_[b];
        if (!block.vertexSpace.Allocate(vertexCount, slot.vertexOffset)) {
            continue;
        }
        if (!block.indexSpace.Allocate(indexCount, slot.firstIndex)) {
            block.vertexSpace.Free(slot.vertexOffset, vertexCount);
            continue;
        }
        slot.block = static_cast<int>(b);
        slot.vertexCount = vertexCount;
        slot.indexCount = indexCount;
        return true;
    }
    return false;
}

bool Gta5GeometryArena::AddBlock(SDL_GPUDevice* device) {
    SDL_GPUBufferCreateInfo info = {};
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    info.size = kBlockVertices * kVertexStride;
    SDL_GPUBuffer* vertices = SDL_CreateGPUBuffer(device, &info);
    info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    info.size = kBlockIndices * sizeof(std::uint16_t);
    SDL_GPUBuffer* indices = SDL_CreateGPUBuffer(device, &info);
    if (!vertices || !indices) {
        if (vertices) SDL_ReleaseGPUBuffer(device, vertices);
        if (indices) SDL_ReleaseGPUBuffer(device, indices);
        return false;
    }
    blocks_.push_back({vertices, indices, Gta5RangeAllocator(kBlockVertices),
                       Gta5RangeAllocator(kBlockIndices)});
    return true;
}

void Gta5GeometryArena::Free(const Gta5ArenaSlot& slot) {
    if (slot.block < 0 || slot.block >= static_cast<int>(blocks_.size())) {
        return;
    }
    Block& block = blocks_[static_cast<std::size_t>(slot.block)];
    block.vertexSpace.Free(slot.vertexOffset, slot.vertexCount);
    block.indexSpace.Free(slot.firstIndex, slot.indexCount);
}

SDL_GPUBuffer* Gta5GeometryArena::Vertices(int block) const {
    return blocks_[static_cast<std::size_t>(block)].vertices;
}

SDL_GPUBuffer* Gta5GeometryArena::Indices(int block) const {
    return blocks_[static_cast<std::size_t>(block)].indices;
}

}  // namespace sdl3cpp::services::impl
