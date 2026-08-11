#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_gamepad_button_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

WorkflowInputGamepadButtonPressedStep::WorkflowInputGamepadButtonPressedStep(
    std::shared_ptr<IInputService> inputService,
    std::shared_ptr<ILogger> logger)
    : inputService_(std::move(inputService)),
      logger_(std::move(logger)) {}

std::string WorkflowInputGamepadButtonPressedStep::GetPluginId() const {
    return "input.gamepad.button.pressed";
}

void WorkflowInputGamepadButtonPressedStep::Execute(const WorkflowStepDefinition& step,
                                                     WorkflowContext& context) {
    if (!inputService_) {
        throw std::runtime_error("input.gamepad.button.pressed requires an IInputService");
    }

    WorkflowStepIoResolver resolver;
    const std::string buttonKey = resolver.GetRequiredInputKey(step, "button");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "is_pressed");

    const auto* buttonStr = context.TryGet<std::string>(buttonKey);
    if (!buttonStr) {
        throw std::runtime_error("input.gamepad.button.pressed missing button input");
    }

    SDL_GamepadButton button = SDL_GetGamepadButtonFromString(buttonStr->c_str());
    if (button == SDL_GAMEPAD_BUTTON_INVALID) {
        throw std::runtime_error("input.gamepad.button.pressed unknown button: " + *buttonStr);
    }

    // Query active gamepad - iterate through available gamepads
    int gamepadCount = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&gamepadCount);

    bool isPressed = false;
    if (gamepads && gamepadCount > 0) {
        // Use first active gamepad
        SDL_Gamepad* gamepad = SDL_OpenGamepad(gamepads[0]);
        if (gamepad) {
            isPressed = SDL_GetGamepadButton(gamepad, button);
        }
        SDL_free(gamepads);
    }

    context.Set(outputKey, isPressed);

    if (logger_) {
        logger_->Trace("WorkflowInputGamepadButtonPressedStep", "Execute",
                       "button=" + *buttonStr +
                           ", is_pressed=" + std::string(isPressed ? "true" : "false") +
                           ", output=" + outputKey,
                       "Checked gamepad button state");
    }
}

}  // namespace sdl3cpp::services::impl
