#include "services/interfaces/workflow/input/workflow_input_poll_all_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_keyboard_poll_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_mouse_poll_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_gamepad_poll_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_axis_combine_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_button_combine_step.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowInputPollAllStep::WorkflowInputPollAllStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputPollAllStep::GetPluginId() const {
    return "input.poll_all";
}

float WorkflowInputPollAllStep::ApplyDeadzone(float value, float deadzone) {
    // Kept for backward compatibility; real logic now in WorkflowInputAxisCombineStep
    float clamped = std::max(-1.0f, std::min(1.0f, value));
    if (std::abs(clamped) < deadzone) {
        return 0.0f;
    }
    if (clamped > 0.0f) {
        return (clamped - deadzone) / (1.0f - deadzone);
    }
    return (clamped + deadzone) / (1.0f - deadzone);
}

void WorkflowInputPollAllStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowInputPollAllStep", "Execute",
                       "Delegating to atomic input steps");
    }

    try {
        // Delegate to atomic steps in sequence:
        // 1. Poll raw hardware state
        WorkflowInputKeyboardPollStep(logger_).Execute(step, context);
        WorkflowInputMousePollStep(logger_).Execute(step, context);
        WorkflowInputGamepadPollStep(logger_).Execute(step, context);

        // 2. Combine raw state into logical inputs
        WorkflowInputAxisCombineStep(logger_).Execute(step, context);
        WorkflowInputButtonCombineStep(logger_).Execute(step, context);

        context.Set<bool>("input.poll_complete", true);

        if (logger_) {
            logger_->Info("input.poll_all: Input aggregation complete (delegated)");
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("input.poll_all: " + std::string(e.what()));
        }
        context.Set<bool>("input.poll_complete", false);
        context.Set<std::string>("input.poll_error", e.what());
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
