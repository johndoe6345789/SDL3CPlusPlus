#include "services/interfaces/workflow/input/workflow_input_mouse_poll_step.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowInputMousePollStep::WorkflowInputMousePollStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputMousePollStep::GetPluginId() const {
    return "input.mouse.poll";
}

void WorkflowInputMousePollStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowInputMousePollStep", "Execute", "Entry");
    }

    // Absolute position and button state
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    uint32_t mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);

    context.Set<float>("input.mouse.x", mouseX);
    context.Set<float>("input.mouse.y", mouseY);
    context.Set<bool>("input.mouse.left", (mouseButtons & SDL_BUTTON_LMASK) != 0);
    context.Set<bool>("input.mouse.right", (mouseButtons & SDL_BUTTON_RMASK) != 0);
    context.Set<bool>("input.mouse.middle", (mouseButtons & SDL_BUTTON_MMASK) != 0);

    // Relative motion since last poll
    float relX = 0.0f;
    float relY = 0.0f;
    SDL_GetRelativeMouseState(&relX, &relY);

    context.Set<float>("input.mouse.rel_x", relX);
    context.Set<float>("input.mouse.rel_y", relY);

    if (logger_) {
        logger_->Debug("input.mouse.poll: pos=(" +
                       std::to_string(static_cast<int>(mouseX)) + ", " +
                       std::to_string(static_cast<int>(mouseY)) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
