#include "services/interfaces/workflow/graphics/workflow_video_record_begin_step.hpp"
#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

namespace sdl3cpp::services::impl {

WorkflowVideoRecordBeginStep::WorkflowVideoRecordBeginStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowVideoRecordBeginStep::GetPluginId() const {
    return "video.record.begin";
}

// After frame.gpu.begin_offscreen, before anything draws to the
// swapchain. On a frame that is due, postfx_swapchain_texture points
// at the recorder's target until video.record.end puts it back.
void WorkflowVideoRecordBeginStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    VideoRecorder& rec = GetVideoRecorder(context, step, logger_);
    if (rec.done || context.GetBool("frame_skip", false)) return;
    if (!rec.started && !StartVideoRecorder(rec, context)) return;

    WatchVideoAudio(rec, context);
    AttachVideoFence(rec, context);
    CollectVideoReadbacks(rec, false);
    const std::int64_t pts = DueVideoPts(rec, context);
    if (pts < 0) return;
    if (!FreeVideoReadback(rec)) {
        ++rec.dropped;
        return;
    }

    const char* key        = "postfx_swapchain_texture";
    auto* swapchain        = context.Get<SDL_GPUTexture*>(key, nullptr);
    SDL_GPUTexture* target = EnsureVideoTarget(
        rec, context.Get<SDL_Window*>("sdl_window", nullptr),
        context.Get<uint32_t>("frame_width", 0),
        context.Get<uint32_t>("frame_height", 0));
    if (!swapchain || !target) return;

    rec.swapchain  = swapchain;
    rec.capturePts = pts;
    context.Set<SDL_GPUTexture*>(key, target);
}

}  // namespace sdl3cpp::services::impl
