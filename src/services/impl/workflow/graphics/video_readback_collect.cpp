#include "services/interfaces/workflow/graphics/video_recorder.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

VideoReadback* Oldest(VideoRecorder& rec) {
    VideoReadback* oldest = nullptr;
    for (VideoReadback& slot : rec.readbacks) {
        if (slot.pending && (!oldest || slot.pts < oldest->pts)) {
            oldest = &slot;
        }
    }
    return oldest;
}

}  // namespace

void AttachVideoFence(VideoRecorder& rec, WorkflowContext& context) {
    auto* fence = context.Get<SDL_GPUFence*>(kGpuSubmitFenceKey, nullptr);
    if (!fence) return;
    context.Remove(kGpuSubmitFenceKey);
    for (VideoReadback& slot : rec.readbacks) {
        if (slot.pending && !slot.fence) {
            slot.fence = std::exchange(fence, nullptr);
            break;
        }
    }
    if (fence) SDL_ReleaseGPUFence(rec.device, fence);
}

// Oldest first: the encoder refuses a pts older than its last one.
void CollectVideoReadbacks(VideoRecorder& rec, bool wait) {
    if (wait) SDL_WaitForGPUIdle(rec.device);
    while (VideoReadback* slot = Oldest(rec)) {
        const bool ready =
            slot->fence && SDL_QueryGPUFence(rec.device, slot->fence);
        if (!wait && !ready) return;
        DeliverVideoReadback(rec, *slot);
    }
}

}  // namespace sdl3cpp::services::impl
