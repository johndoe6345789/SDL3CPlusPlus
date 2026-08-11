#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Poll SDL mouse state (position, buttons, relative motion) and write to context.
 *
 * Plugin ID: "input.mouse.poll"
 *
 * Reads the current mouse position, button state, and relative motion
 * from SDL and stores them in the workflow context.
 *
 * Context Output:
 *   - input.mouse.x (float): Absolute mouse X position
 *   - input.mouse.y (float): Absolute mouse Y position
 *   - input.mouse.left (bool): Left mouse button pressed
 *   - input.mouse.right (bool): Right mouse button pressed
 *   - input.mouse.middle (bool): Middle mouse button pressed
 *   - input.mouse.rel_x (float): Relative mouse X motion since last poll
 *   - input.mouse.rel_y (float): Relative mouse Y motion since last poll
 */
class WorkflowInputMousePollStep final : public IWorkflowStep {
public:
    explicit WorkflowInputMousePollStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
