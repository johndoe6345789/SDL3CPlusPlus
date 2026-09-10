#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Composites the HDR target (plus SSAO and bloom) onto the swapchain.
 *
 * Publishes the outcome under kPostfxCompositeStateKey so the follow-up steps
 * (`postfx.overlay_fps`, `gpu.screenshot_capture`, `gpu.command_buffer_submit`
 * and `postfx.composite_finish`) can tell a completed frame from a skipped one.
 */
class WorkflowPostfxCompositeDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxCompositeDrawStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
