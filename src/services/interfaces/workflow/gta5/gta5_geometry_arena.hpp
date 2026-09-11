#pragma once

#include "services/interfaces/workflow/gta5/gta5_range_allocator.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Where a submesh lives in the arena. block < 0: nowhere.
struct Gta5ArenaSlot {
    int block{-1};
    std::uint32_t vertexOffset{0};
    std::uint32_t vertexCount{0};
    std::uint32_t firstIndex{0};
    std::uint32_t indexCount{0};
};

/// Map geometry packed into a few large vertex and index buffers.
///
/// A buffer per submesh made every draw bind two new buffers, and SDL's
/// Vulkan backend tracks each buffer a command buffer uses by scanning
/// the ones it already has: 6,000 of them a frame cost ~5 ms in that
/// scan alone. Packed, a frame binds one pair per block. Every vertex
/// is the 40-byte position_uv_lmuv_normal layout.
class Gta5GeometryArena {
public:
    static constexpr std::uint32_t kVertexStride = 40;

    /// Place and upload a submesh's vertices and 16-bit indices, in one
    /// block, adding a block when none has room.
    bool Upload(SDL_GPUDevice* device, const void* vertices,
                std::uint32_t vertexCount, const std::uint16_t* indices,
                std::uint32_t indexCount, Gta5ArenaSlot& slot);
    /// Give a slot's space back. The GPU may still be drawing from it
    /// for a frame, which is fine: a later upload to it is ordered after.
    void Free(const Gta5ArenaSlot& slot);

    SDL_GPUBuffer* Vertices(int block) const;
    SDL_GPUBuffer* Indices(int block) const;

private:
    struct Block {
        SDL_GPUBuffer* vertices{nullptr};
        SDL_GPUBuffer* indices{nullptr};
        Gta5RangeAllocator vertexSpace;
        Gta5RangeAllocator indexSpace;
    };
    bool Place(std::uint32_t vertexCount, std::uint32_t indexCount,
               Gta5ArenaSlot& slot);
    bool AddBlock(SDL_GPUDevice* device);

    std::vector<Block> blocks_;
};

}  // namespace sdl3cpp::services::impl
