#include "services/interfaces/workflow/graphics/video_recorder.hpp"

namespace sdl3cpp::services::impl {
namespace {

bool EnsureBuffer(VideoRecorder& rec, VideoReadback& slot,
                  std::uint32_t bytes) {
    if (slot.buffer && slot.capacity >= bytes) return true;
    if (slot.buffer) SDL_ReleaseGPUTransferBuffer(rec.device, slot.buffer);
    SDL_GPUTransferBufferCreateInfo info = {};
    info.usage    = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size     = bytes;
    slot.buffer   = SDL_CreateGPUTransferBuffer(rec.device, &info);
    slot.capacity = slot.buffer ? bytes : 0;
    return slot.buffer != nullptr;
}

}  // namespace

VideoReadback* FreeVideoReadback(VideoRecorder& rec) {
    for (VideoReadback& slot : rec.readbacks) {
        if (!slot.pending) return &slot;
    }
    return nullptr;
}

void QueueVideoReadback(VideoRecorder& rec, VideoReadback& slot,
                        SDL_GPUCommandBuffer* cmd) {
    const std::uint32_t w = rec.targetWidth, h = rec.targetHeight;
    SDL_GPUCopyPass* copy = EnsureBuffer(rec, slot, w * h * 4)
                                ? SDL_BeginGPUCopyPass(cmd)
                                : nullptr;
    if (!copy) {
        ++rec.dropped;
        return;
    }
    SDL_GPUTextureRegion from     = {};
    from.texture                  = rec.target;
    from.w                        = w;
    from.h                        = h;
    from.d                        = 1;
    SDL_GPUTextureTransferInfo to = {};
    to.transfer_buffer            = slot.buffer;
    SDL_DownloadFromGPUTexture(copy, &from, &to);
    SDL_EndGPUCopyPass(copy);
    slot.fence   = nullptr;
    slot.pending = true;
    slot.width   = w;
    slot.height  = h;
    slot.pts     = rec.capturePts;
}

}  // namespace sdl3cpp::services::impl
