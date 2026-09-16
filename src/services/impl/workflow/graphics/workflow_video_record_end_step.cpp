#include "services/interfaces/workflow/graphics/workflow_video_record_end_step.hpp"
#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

namespace sdl3cpp::services::impl {

WorkflowVideoRecordEndStep::WorkflowVideoRecordEndStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowVideoRecordEndStep::GetPluginId() const {
    return "video.record.end";
}

// Last before gpu.command_buffer_submit: puts the swapchain back,
// shows the recorded frame on it, and queues the frame's download.
void WorkflowVideoRecordEndStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    VideoRecorder& rec = GetVideoRecorder(context, step, logger_);
    if (!rec.swapchain) return;
    SDL_GPUTexture* swapchain = rec.swapchain;
    rec.swapchain             = nullptr;
    context.Set<SDL_GPUTexture*>("postfx_swapchain_texture", swapchain);

    const char* key     = "gpu_command_buffer";
    auto* cmd           = context.Get<SDL_GPUCommandBuffer*>(key, nullptr);
    VideoReadback* slot = FreeVideoReadback(rec);
    if (!cmd || !slot || context.GetBool("frame_skip", false)) return;

    PresentVideoTarget(rec, cmd, swapchain);
    QueueVideoReadback(rec, *slot, cmd);
    rec.nextPts = rec.capturePts + 1;
    context.Set<bool>(kGpuSubmitFenceWantedKey, true);
}

}  // namespace sdl3cpp::services::impl
