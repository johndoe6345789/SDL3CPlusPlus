#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_upload.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRacerHudStep::WorkflowRacerHudStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerHudStep::GetPluginId() const {
    return "racer.hud.text";
}

void WorkflowRacerHudStep::Execute(const WorkflowStepDefinition&,
                                   WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->loaded) return;
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* res =
        context.Get<GpuTextOverlayResources*>("overlay_fps_resources", nullptr);
    if (!cmd || !res) return;
    const std::string text = FormatRacerHud(state_->race, state_->pod);
    // The overlay texture keeps its contents, so only changes upload.
    if (text == shown_) return;
    const SDL_Color amber{255, 220, 50, 255};
    if (UploadGpuTextOverlayText(*res, cmd, text.c_str(), amber)) {
        shown_ = text;
    } else if (logger_) {
        logger_->Trace("racer.hud.text: overlay upload failed");
    }
}

}  // namespace sdl3cpp::services::impl
