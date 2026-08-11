#include "services/interfaces/workflow/input/workflow_input_keyboard_poll_step.hpp"

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowInputKeyboardPollStep::WorkflowInputKeyboardPollStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputKeyboardPollStep::GetPluginId() const {
    return "input.keyboard.poll";
}

void WorkflowInputKeyboardPollStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowInputKeyboardPollStep", "Execute", "Entry");
    }

    int numKeys = 0;
    const bool* keyboardState = SDL_GetKeyboardState(&numKeys);
    if (!keyboardState) {
        throw std::runtime_error("input.keyboard.poll: Failed to get keyboard state");
    }

    // Store raw keyboard state as JSON for downstream combine steps
    nlohmann::json keyState = nlohmann::json::object();
    for (int i = 0; i < numKeys; ++i) {
        if (keyboardState[i]) {
            const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(i));
            if (name && name[0] != '\0') {
                keyState[name] = true;
            }
        }
    }

    context.Set<nlohmann::json>("input.keyboard.state", std::move(keyState));
    context.Set<int>("input.keyboard.num_keys", numKeys);

    if (logger_) {
        logger_->Debug("input.keyboard.poll: " + std::to_string(numKeys) + " keys polled");
    }
}

}  // namespace sdl3cpp::services::impl
