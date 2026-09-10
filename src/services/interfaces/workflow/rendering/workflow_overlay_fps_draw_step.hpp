#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.fps_draw
 *
 * Draws the FPS overlay quad directly onto `gpu_swapchain_texture` using a
 * LOADOP_LOAD pass, so whatever the rest of the frame already drew there is
 * preserved.  Does not submit the command buffer — a later step in the
 * "Q3 Overlay" sub-workflow (overlay.sw.end) owns the final submit.
 */
class WorkflowOverlayFpsDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlayFpsDrawStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
