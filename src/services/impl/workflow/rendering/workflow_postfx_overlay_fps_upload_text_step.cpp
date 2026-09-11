#include "services/interfaces/workflow/rendering/workflow_postfx_overlay_fps_upload_text_step.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_upload.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowPostfxOverlayFpsUploadTextStep::WorkflowPostfxOverlayFpsUploadTextStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxOverlayFpsUploadTextStep::GetPluginId() const {
    return "postfx.overlay_fps_upload_text";
}

void WorkflowPostfxOverlayFpsUploadTextStep::Execute(
    const WorkflowStepDefinition&, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }
    if (context.GetString(kPostfxCompositeStateKey) !=
        kPostfxCompositeStateDrawn) {
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* res = context.Get<GpuTextOverlayResources*>(
        "postfx_overlay_resources", nullptr);
    if (!cmd || !res || !res->surface || !res->renderer || !res->transfer) {
        return;
    }

    const float fps = meter_.Update(SDL_GetTicksNS());

    char text[32];
    std::snprintf(text, sizeof(text), "FPS: %.0f", fps);

    UploadGpuTextOverlayText(*res, cmd, text, SDL_Color{255, 220, 50, 255});
}

}  // namespace sdl3cpp::services::impl
