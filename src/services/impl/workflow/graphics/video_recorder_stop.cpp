#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

void ReleaseGpuObjects(VideoRecorder& rec) {
    for (VideoReadback& slot : rec.readbacks) {
        if (slot.buffer) {
            SDL_ReleaseGPUTransferBuffer(rec.device, slot.buffer);
        }
        slot = VideoReadback{};
    }
    if (rec.target) SDL_ReleaseGPUTexture(rec.device, rec.target);
    rec.target = nullptr;
}

}  // namespace

void StopVideoRecorder(VideoRecorder& rec, WorkflowContext& context) {
    if (!rec.started) return;
    AttachVideoFence(rec, context);
    CollectVideoReadbacks(rec, true);
    if (rec.audio) rec.audio->SetActive(false);  // the last pass in
    rec.worker.Finish();
    rec.audio.reset();
    ReleaseGpuObjects(rec);
    rec.started = false;
    rec.done    = true;
    if (!rec.logger) return;
    rec.logger->Info("video.record: wrote " +
                     std::to_string(rec.worker.Written()) + " frames to " +
                     rec.settings.path + ", dropped " +
                     std::to_string(rec.dropped));
}

}  // namespace sdl3cpp::services::impl
