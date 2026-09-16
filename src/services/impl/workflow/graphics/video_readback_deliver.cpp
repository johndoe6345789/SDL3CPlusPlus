#include "services/interfaces/workflow/graphics/video_recorder.hpp"

#include <cstring>
#include <utility>

namespace sdl3cpp::services::impl {

void DeliverVideoReadback(VideoRecorder& rec, VideoReadback& slot) {
    const std::size_t bytes = std::size_t(slot.width) * slot.height * 4;
    void* mapped = SDL_MapGPUTransferBuffer(rec.device, slot.buffer, false);
    if (mapped) {
        VideoFrame frame;
        frame.pixels = std::make_unique<std::uint8_t[]>(bytes);
        std::memcpy(frame.pixels.get(), mapped, bytes);
        SDL_UnmapGPUTransferBuffer(rec.device, slot.buffer);
        frame.width  = int(slot.width);
        frame.height = int(slot.height);
        frame.bgra   = rec.bgra;
        frame.pts    = slot.pts;
        if (!rec.worker.Push(std::move(frame))) ++rec.dropped;
    }
    if (slot.fence) SDL_ReleaseGPUFence(rec.device, slot.fence);
    slot.fence   = nullptr;
    slot.pending = false;
}

}  // namespace sdl3cpp::services::impl
