#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Read raw input sources from context, combine into logical button states.
 *
 * Plugin ID: "input.button.combine"
 *
 * Reads button bindings from the input aggregation config (loaded from file or
 * context). For each button binding, checks the raw input values that were
 * previously written by keyboard/mouse/gamepad poll steps. If any source
 * reports pressed, the logical button is pressed (OR logic).
 *
 * Parameters:
 *   - config_path (string, optional): Path to input_aggregation.json
 *
 * Context Input:
 *   - input.aggregation.config (nlohmann::json, optional): Config loaded into context
 *   - input.keyboard.state (nlohmann::json): Keyboard state from keyboard poll
 *   - input.mouse.left, input.mouse.right, input.mouse.middle (bool): Mouse buttons
 *   - input.gamepad.button_* (bool): Gamepad buttons from gamepad poll
 *   - input.gamepad.trigger_left, input.gamepad.trigger_right (float): Trigger axes
 *   - input.gamepad.connected (bool): Whether gamepad is available
 *
 * Context Output:
 *   - Configured output keys (bool): Logical button states
 *     e.g., input.jump_pressed, input.fire_pressed, input.menu_pressed
 */
class WorkflowInputButtonCombineStep final : public IWorkflowStep {
public:
    explicit WorkflowInputButtonCombineStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
