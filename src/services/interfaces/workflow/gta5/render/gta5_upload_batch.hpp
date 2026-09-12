#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Every map upload of a frame, copied into one staging buffer and
/// submitted as one copy pass.
///
/// A transfer buffer and a command buffer per texture and per submesh
/// made a streaming frame pay the driver for dozens of allocations and
/// queue submits: 5-14 ms spikes at the submit, on top of the copies.
/// Here staging is one persistent buffer, cycled each frame, and
/// gta5.tiles.cull submits it all before the frame renders.
class Gta5UploadBatch {
public:
    /// Bytes staged since the last flush: what the finish loop budgets.
    std::uint64_t Pending() const { return used_; }

    bool StageBuffer(SDL_GPUDevice* device, const void* data,
                     std::uint32_t size, SDL_GPUBuffer* target,
                     std::uint32_t offset);
    /// One mip level, rows tightly packed as GTA stores them.
    bool StageTexture(SDL_GPUDevice* device, const void* data,
                      std::uint32_t size, SDL_GPUTexture* texture,
                      std::uint32_t level, std::uint32_t width,
                      std::uint32_t height);

    /// Submit everything staged, in one copy pass. Staging past the
    /// buffer's room flushes early rather than failing.
    void Flush(SDL_GPUDevice* device);

private:
    struct Copy {
        SDL_GPUBuffer* buffer{nullptr};
        SDL_GPUTexture* texture{nullptr};
        std::uint32_t target{0};  // buffer offset, or mip level
        std::uint32_t width{0};
        std::uint32_t height{0};
        std::uint32_t source{0};
        std::uint32_t size{0};
    };
    std::uint8_t* Reserve(SDL_GPUDevice* device, std::uint32_t size,
                          std::uint32_t& at);

    SDL_GPUTransferBuffer* transfer_{nullptr};
    std::uint8_t* mapped_{nullptr};
    std::uint32_t used_{0};
    std::vector<Copy> copies_;
};

}  // namespace sdl3cpp::services::impl
