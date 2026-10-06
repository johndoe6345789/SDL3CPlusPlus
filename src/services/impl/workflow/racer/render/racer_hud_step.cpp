#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRacerHudStep::WorkflowRacerHudStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

WorkflowRacerHudStep::~WorkflowRacerHudStep() {
    DestroyRacerHudOverlay(hud_);
}

std::string WorkflowRacerHudStep::GetPluginId() const {
    return "racer.hud.text";
}

void WorkflowRacerHudStep::Execute(const WorkflowStepDefinition&,
                                   WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->loaded) return;
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    // The engine's text overlay lends its pipeline and sampler.
    auto* res =
        context.Get<GpuTextOverlayResources*>("overlay_fps_resources", nullptr);
    if (!cmd || !swapchain || !res || !res->pipeline) return;
    if (!hud_.texture && !CreateRacerHudOverlay(res->device, hud_)) {
        if (logger_) logger_->Warn("racer.hud.text: HUD creation failed");
        return;
    }
    const std::string text = FormatRacerHud(state_->race, state_->pod);
    if (text != shown_ || !hud_.quadUploaded) {
        UploadRacerHud(hud_, cmd, text,
                       static_cast<int>(context.Get<uint32_t>("frame_width",
                                                              1920u)),
                       static_cast<int>(context.Get<uint32_t>("frame_height",
                                                              1080u)));
        shown_ = text;
    }
    DrawRacerHud(hud_, cmd, swapchain, res->pipeline, res->sampler);
}

}  // namespace sdl3cpp::services::impl
