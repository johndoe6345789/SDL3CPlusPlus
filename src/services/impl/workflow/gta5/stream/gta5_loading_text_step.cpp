#include "services/interfaces/workflow/gta5/stream/gta5_loading_text_step.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_upload.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5LoadingTextStep::WorkflowGta5LoadingTextStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGta5LoadingTextStep::GetPluginId() const {
    return "gta5.loading.text";
}

void WorkflowGta5LoadingTextStep::Execute(const WorkflowStepDefinition&,
                                          WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    if (context.GetString(kPostfxCompositeStateKey) !=
        kPostfxCompositeStateDrawn) {
        return;
    }
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* res = context.Get<GpuTextOverlayResources*>(
        "postfx_overlay_resources", nullptr);
    if (!cmd || !res) return;

    // Blank until the hold has something to say, and blank again after:
    // the overlay texture is never left holding whatever it was created
    // with.
    // Loading progress while there is any, and the clock after it.
    std::string text = context.GetString("gta5.loading.text", "");
    if (text.empty()) {
        // The clock, and for a few seconds in a car the station before it.
        text = context.GetString("gta5.clock.text", "");
        const std::string radio = context.GetString("gta5.radio.text", "");
        if (!radio.empty()) text = radio + "  " + text;
    }
    if (uploaded_ && text == shown_) return;
    const SDL_Color amber{255, 220, 50, 255};
    const bool ok = UploadGpuTextOverlayText(*res, cmd, text.c_str(), amber);
    if (logger_) {
        // Trace, not Info: the percentage changes a hundred times a load.
        logger_->Trace("WorkflowGta5LoadingTextStep", "Execute",
                       "text='" + text + "'", ok ? "uploaded" : "FAILED");
    }
    if (ok) {
        uploaded_ = true;
        shown_ = text;
    }
}

}  // namespace sdl3cpp::services::impl
