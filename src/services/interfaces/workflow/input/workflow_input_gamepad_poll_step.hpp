#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Poll SDL gamepad axes and buttons, write to context.
 *
 * Plugin ID: "input.gamepad.poll"
 *
 * Opens the first available joystick/gamepad, reads all standard axes
 * and buttons, then stores normalized values in the workflow context.
 * If no gamepad is connected, sets input.gamepad.connected to false
 * and skips axis/button reads gracefully.
 *
 * Axis values are normalized to [-1.0, 1.0] from SDL's [-32768, 32767] range.
 *
 * Context Output:
 *   - input.gamepad.connected (bool): Whether a gamepad is connected
 *   - input.gamepad.left_stick_x (float): Left stick X axis [-1, 1]
 *   - input.gamepad.left_stick_y (float): Left stick Y axis [-1, 1]
 *   - input.gamepad.right_stick_x (float): Right stick X axis [-1, 1]
 *   - input.gamepad.right_stick_y (float): Right stick Y axis [-1, 1]
 *   - input.gamepad.trigger_left (float): Left trigger [0, 1]
 *   - input.gamepad.trigger_right (float): Right trigger [0, 1]
 *   - input.gamepad.button_south (bool): A/Cross button
 *   - input.gamepad.button_east (bool): B/Circle button
 *   - input.gamepad.button_west (bool): X/Square button
 *   - input.gamepad.button_north (bool): Y/Triangle button
 *   - input.gamepad.button_left_shoulder (bool): Left bumper
 *   - input.gamepad.button_right_shoulder (bool): Right bumper
 *   - input.gamepad.button_back (bool): Back/Select button
 *   - input.gamepad.button_start (bool): Start button
 */
class WorkflowInputGamepadPollStep final : public IWorkflowStep {
public:
    explicit WorkflowInputGamepadPollStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
