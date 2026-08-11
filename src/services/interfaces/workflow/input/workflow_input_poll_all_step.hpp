#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Convenience orchestrator that delegates to atomic input steps.
 *
 * Plugin ID: "input.poll_all"
 *
 * DEPRECATED: Prefer using atomic steps directly in workflows:
 *   1. input.keyboard.poll  - Poll SDL keyboard state
 *   2. input.mouse.poll     - Poll SDL mouse state
 *   3. input.gamepad.poll   - Poll SDL gamepad state
 *   4. input.axis.combine   - Combine raw inputs into logical axes
 *   5. input.button.combine - Combine raw inputs into logical buttons
 *
 * This step is kept for backward compatibility. It internally instantiates
 * and executes each atomic step in sequence, producing identical context
 * output as before.
 */
class WorkflowInputPollAllStep final : public IWorkflowStep {
public:
    explicit WorkflowInputPollAllStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;

    /**
     * Apply deadzone to axis value (clamp to [0, 1] range after threshold)
     */
    static float ApplyDeadzone(float value, float deadzone);
};

}  // namespace sdl3cpp::services::impl
