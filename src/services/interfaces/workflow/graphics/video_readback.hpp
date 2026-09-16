#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

struct VideoRecorder;

/// A frame on its way back from the GPU. `fence` arrives one frame
/// late: the submit that makes it runs after the download is queued.
struct VideoReadback {
    SDL_GPUTransferBuffer* buffer = nullptr;
    std::uint32_t capacity        = 0;
    SDL_GPUFence* fence           = nullptr;
    bool pending                  = false;
    std::uint32_t width           = 0;
    std::uint32_t height          = 0;
    std::int64_t pts              = 0;
};

/// A slot with no download in flight, or null when all are busy.
VideoReadback* FreeVideoReadback(VideoRecorder& rec);
/// Queues a download of the recorder's target on @p cmd.
void QueueVideoReadback(VideoRecorder& rec, VideoReadback& slot,
                        SDL_GPUCommandBuffer* cmd);
/// Gives the last submit's fence to the download it carried.
void AttachVideoFence(VideoRecorder& rec, WorkflowContext& context);
/// Hands finished downloads to the encoder; @p wait blocks for all.
void CollectVideoReadbacks(VideoRecorder& rec, bool wait);
/// Copies one finished download out, and frees its slot.
void DeliverVideoReadback(VideoRecorder& rec, VideoReadback& slot);

}  // namespace sdl3cpp::services::impl
