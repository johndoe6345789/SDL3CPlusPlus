#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.sw.end_draw
 *
 * Draws the SW overlay's fullscreen quad onto `gpu_swapchain_texture` with a
 * LOADOP_LOAD pass, so the scene already drawn there is preserved underneath.
 */
class WorkflowOverlaySwEndDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwEndDrawStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
