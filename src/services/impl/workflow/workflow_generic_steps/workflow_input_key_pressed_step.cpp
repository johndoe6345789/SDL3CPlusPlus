#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_key_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

WorkflowInputKeyPressedStep::WorkflowInputKeyPressedStep(std::shared_ptr<IInputService> inputService,
                                                         std::shared_ptr<ILogger> logger)
    : inputService_(std::move(inputService)),
      logger_(std::move(logger)) {}

std::string WorkflowInputKeyPressedStep::GetPluginId() const {
    return "input.key.pressed";
}

void WorkflowInputKeyPressedStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!inputService_) {
        throw std::runtime_error("input.key.pressed requires an IInputService");
    }

    WorkflowStepIoResolver resolver;
    const std::string keyCodeKey = resolver.GetRequiredInputKey(step, "key_code");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "is_pressed");

    const auto* keyCodeStr = context.TryGet<std::string>(keyCodeKey);
    if (!keyCodeStr) {
        throw std::runtime_error("input.key.pressed missing key_code input");
    }

    SDL_Keycode keycode = SDL_GetKeyFromName(keyCodeStr->c_str());
    if (keycode == SDLK_UNKNOWN) {
        throw std::runtime_error("input.key.pressed unknown key: " + *keyCodeStr);
    }

    const bool isPressed = inputService_->IsKeyPressed(keycode);
    context.Set(outputKey, isPressed);

    if (logger_) {
        logger_->Trace("WorkflowInputKeyPressedStep", "Execute",
                       "key_code=" + *keyCodeStr +
                           ", is_pressed=" + std::string(isPressed ? "true" : "false") +
                           ", output=" + outputKey,
                       "Checked key state");
    }
}

}  // namespace sdl3cpp::services::impl
