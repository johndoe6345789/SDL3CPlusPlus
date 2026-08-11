#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Read raw input sources from context, apply deadzone/inversion/scaling,
 * write combined axis values.
 *
 * Plugin ID: "input.axis.combine"
 *
 * Reads axis bindings from the input aggregation config (loaded from file or
 * context). For each axis binding, reads the raw input values that were
 * previously written by keyboard/mouse/gamepad poll steps, applies per-source
 * deadzone, inversion, and scaling, then writes the combined axis value to
 * the configured output keys.
 *
 * Parameters:
 *   - config_path (string, optional): Path to input_aggregation.json
 *
 * Context Input:
 *   - input.aggregation.config (nlohmann::json, optional): Config loaded into context
 *   - input.keyboard.state (nlohmann::json): Keyboard state from keyboard poll
 *   - input.mouse.x, input.mouse.y (float): Mouse position from mouse poll
 *   - input.gamepad.* (float): Gamepad axes from gamepad poll
 *   - input.gamepad.connected (bool): Whether gamepad is available
 *
 * Context Output:
 *   - Configured output keys (float): Combined axis values clamped to [-1, 1]
 *     e.g., input.forward_axis, input.right_axis, input.look_yaw_delta
 */
class WorkflowInputAxisCombineStep final : public IWorkflowStep {
public:
    explicit WorkflowInputAxisCombineStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;

    /**
     * Apply deadzone to axis value (clamp to [-1, 1] range after threshold).
     */
    static float ApplyDeadzone(float value, float deadzone);
};

}  // namespace sdl3cpp::services::impl
