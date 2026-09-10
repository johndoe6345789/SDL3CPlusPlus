#include "services/interfaces/workflow/rendering/workflow_postfx_composite_finish_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

WorkflowPostfxCompositeFinishStep::WorkflowPostfxCompositeFinishStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxCompositeFinishStep::GetPluginId() const {
    return "postfx.composite_finish";
}

void WorkflowPostfxCompositeFinishStep::Execute(const WorkflowStepDefinition&,
                                                WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }
    if (context.GetString(kPostfxCompositeStateKey) !=
        kPostfxCompositeStateDrawn) {
        return;
    }

    context.Remove("postfx_swapchain_texture");

    const auto frameNumber = context.Get<uint32_t>("frame_number", 0u);
    context.Set<uint32_t>("frame_number", frameNumber + 1u);

    if (logger_) {
        logger_->Trace("WorkflowPostfxCompositeFinishStep", "Execute",
                       "frame_number=" + std::to_string(frameNumber + 1u),
                       "Retired a composited frame");
    }
}

}  // namespace sdl3cpp::services::impl
