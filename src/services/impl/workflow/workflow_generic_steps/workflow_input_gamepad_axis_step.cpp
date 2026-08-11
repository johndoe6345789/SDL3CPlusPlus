#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_gamepad_axis_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

WorkflowInputGamepadAxisStep::WorkflowInputGamepadAxisStep(std::shared_ptr<IInputService> inputService,
                                                           std::shared_ptr<ILogger> logger)
    : inputService_(std::move(inputService)),
      logger_(std::move(logger)) {}

std::string WorkflowInputGamepadAxisStep::GetPluginId() const {
    return "input.gamepad.axis";
}

void WorkflowInputGamepadAxisStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!inputService_) {
        throw std::runtime_error("input.gamepad.axis requires an IInputService");
    }

    WorkflowStepIoResolver resolver;
    const std::string axisKey = resolver.GetRequiredInputKey(step, "axis");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* axisStr = context.TryGet<std::string>(axisKey);
    if (!axisStr) {
        throw std::runtime_error("input.gamepad.axis missing axis input");
    }

    SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(axisStr->c_str());
    if (axis == SDL_GAMEPAD_AXIS_INVALID) {
        throw std::runtime_error("input.gamepad.axis unknown axis: " + *axisStr);
    }

    // Query active gamepad - iterate through available gamepads
    int gamepadCount = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&gamepadCount);

    double axisValue = 0.0;
    if (gamepads && gamepadCount > 0) {
        // Use first active gamepad
        SDL_Gamepad* gamepad = SDL_OpenGamepad(gamepads[0]);
        if (gamepad) {
            int16_t rawValue = SDL_GetGamepadAxis(gamepad, axis);
            // Normalize from [-32768, 32767] to [-1.0, 1.0]
            axisValue = static_cast<double>(rawValue) / 32767.5;
        }
        SDL_free(gamepads);
    }

    context.Set(outputKey, axisValue);

    if (logger_) {
        logger_->Trace("WorkflowInputGamepadAxisStep", "Execute",
                       "axis=" + *axisStr +
                           ", value=" + std::to_string(axisValue) +
                           ", output=" + outputKey,
                       "Retrieved gamepad axis value");
    }
}

}  // namespace sdl3cpp::services::impl
