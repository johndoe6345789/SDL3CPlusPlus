#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_screenshot_step.hpp"

#include <SDL3/SDL_surface.h>

namespace sdl3cpp::services::impl {

WorkflowOverlaySwEndScreenshotStep::WorkflowOverlaySwEndScreenshotStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowOverlaySwEndScreenshotStep::GetPluginId() const {
    return "overlay.sw.end_screenshot";
}

void WorkflowOverlaySwEndScreenshotStep::Execute(const WorkflowStepDefinition&,
                                                 WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) ||
        !context.GetBool("overlay.ready", false)) {
        return;
    }

    const auto* path = context.TryGet<std::string>("screenshot_output_path");
    if (!path || path->empty()) {
        return;
    }

    auto* surface = context.Get<SDL_Surface*>("overlay.surface", nullptr);
    if (surface && SDL_SaveBMP(surface, path->c_str())) {
        context.Set<bool>("screenshot_saved", true);
    }
    context.Set<std::string>("screenshot_output_path", std::string(""));
}

}  // namespace sdl3cpp::services::impl
