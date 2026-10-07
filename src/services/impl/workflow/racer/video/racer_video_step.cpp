#include "services/interfaces/workflow/racer/video/racer_video_step.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kMaxCatchUp = 4;  // frames decoded per tick when behind

}  // namespace

WorkflowRacerVideoStep::WorkflowRacerVideoStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

WorkflowRacerVideoStep::~WorkflowRacerVideoStep() {
    Stop();
    DestroyRacerPanel(panel_);
}

std::string WorkflowRacerVideoStep::GetPluginId() const {
    return "racer.video.play";
}

void WorkflowRacerVideoStep::Execute(const WorkflowStepDefinition& step,
                                     WorkflowContext& context) {
    RacerFlow& flow = state_->flow;
    if (flow.phase != RacerPhase::Cutscene) {
        if (decoder_) Stop();
        return;
    }
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    auto* res =
        context.Get<GpuTextOverlayResources*>("overlay_fps_resources", nullptr);
    if (context.GetBool("frame_skip", false) || !cmd || !swapchain || !res ||
        !res->pipeline) {
        return;
    }
    const float w =
        static_cast<float>(context.Get<uint32_t>("frame_width", 1920u));
    const float h =
        static_cast<float>(context.Get<uint32_t>("frame_height", 1080u));
    bool ended = flow.skipVideo;
    flow.skipVideo = false;
    if (!ended && !decoder_ && !StartNext(step, res->device, cmd, w / h)) {
        ended = true;
    }
    bool fresh = false;
    if (!ended) {
        clock_ += std::min(0.1, context.Get<double>("frame.delta_time", 0.0));
        const int due = 1 + static_cast<int>(clock_ *
                                             decoder_->FramesPerSecond());
        for (int k = 0; shown_ < due && k < kMaxCatchUp; ++k, ++shown_) {
            if (!decoder_->NextFrame(rgba_)) {
                ended = true;
                break;
            }
            fresh = true;
        }
        FeedAudio();
    }
    if (ended) {
        Stop();
        if (flow.videos.empty()) flow.phase = flow.afterVideos;
        return;
    }
    if (fresh) UploadRacerPanelPixels(panel_, cmd, rgba_.data());
    DrawRacerPanel(panel_, cmd, swapchain, res->pipeline, res->sampler);
}

}  // namespace sdl3cpp::services::impl
