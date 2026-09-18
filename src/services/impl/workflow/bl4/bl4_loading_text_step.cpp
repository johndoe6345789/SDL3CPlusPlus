#include "services/interfaces/workflow/bl4/bl4_loading_text_step.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_upload.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowBl4LoadingTextStep::WorkflowBl4LoadingTextStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBl4LoadingTextStep::GetPluginId() const { return "bl4.loading.text"; }

void WorkflowBl4LoadingTextStep::Execute(const WorkflowStepDefinition&,
                                         WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    if (context.GetString(kPostfxCompositeStateKey) != kPostfxCompositeStateDrawn) return;
    const std::string text = context.GetString("bl4.loading.text", "");
    // Unlike gta5's, this never uploads a blank: it shares the overlay
    // with postfx.overlay_fps_upload_text, which runs before it, so
    // saying nothing hands the overlay back to the FPS counter.
    if (text.empty()) {
        shown_.clear();
        return;
    }
    if (text == shown_) return;
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* res = context.Get<GpuTextOverlayResources*>("postfx_overlay_resources", nullptr);
    if (!cmd || !res) return;

    const SDL_Color amber{255, 220, 50, 255};
    if (UploadGpuTextOverlayText(*res, cmd, text.c_str(), amber)) shown_ = text;
    else if (logger_) logger_->Trace("bl4.loading.text: upload failed");
}

}  // namespace sdl3cpp::services::impl
