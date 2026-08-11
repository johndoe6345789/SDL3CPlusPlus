#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_button_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

WorkflowInputMouseButtonPressedStep::WorkflowInputMouseButtonPressedStep(
    std::shared_ptr<IInputService> inputService,
    std::shared_ptr<ILogger> logger)
    : inputService_(std::move(inputService)),
      logger_(std::move(logger)) {}

std::string WorkflowInputMouseButtonPressedStep::GetPluginId() const {
    return "input.mouse.button.pressed";
}

void WorkflowInputMouseButtonPressedStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!inputService_) {
        throw std::runtime_error("input.mouse.button.pressed requires an IInputService");
    }

    WorkflowStepIoResolver resolver;
    const std::string buttonKey = resolver.GetRequiredInputKey(step, "button");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "is_pressed");

    const auto* buttonStr = context.TryGet<std::string>(buttonKey);
    if (!buttonStr) {
        throw std::runtime_error("input.mouse.button.pressed missing button input");
    }

    uint8_t button = 0;
    if (*buttonStr == "left") {
        button = SDL_BUTTON_LEFT;
    } else if (*buttonStr == "right") {
        button = SDL_BUTTON_RIGHT;
    } else if (*buttonStr == "middle") {
        button = SDL_BUTTON_MIDDLE;
    } else if (*buttonStr == "x1") {
        button = SDL_BUTTON_X1;
    } else if (*buttonStr == "x2") {
        button = SDL_BUTTON_X2;
    } else {
        throw std::runtime_error("input.mouse.button.pressed unknown button: " + *buttonStr);
    }

    const bool isPressed = inputService_->IsMouseButtonPressed(button);
    context.Set(outputKey, isPressed);

    if (logger_) {
        logger_->Trace("WorkflowInputMouseButtonPressedStep", "Execute",
                       "button=" + *buttonStr +
                           ", is_pressed=" + std::string(isPressed ? "true" : "false") +
                           ", output=" + outputKey,
                       "Checked mouse button state");
    }
}

}  // namespace sdl3cpp::services::impl
