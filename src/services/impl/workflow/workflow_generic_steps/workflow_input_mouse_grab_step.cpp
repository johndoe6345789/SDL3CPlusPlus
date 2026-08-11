#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_grab_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowInputMouseGrabStep::WorkflowInputMouseGrabStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputMouseGrabStep::GetPluginId() const {
    return "input.mouse.grab";
}

void WorkflowInputMouseGrabStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    SDL_Window* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!window) {
        if (logger_) logger_->Error("input.mouse.grab: No SDL window in context");
        return;
    }

    // Prefer context key input over static parameter so the DAG can drive grab per-frame.
    bool grab = true;
    auto enabledIt = step.inputs.find("enabled");
    if (enabledIt != step.inputs.end()) {
        const auto* v = context.TryGet<bool>(enabledIt->second);
        if (v) grab = *v;
    } else {
        WorkflowStepParameterResolver paramResolver;
        if (const auto* p = paramResolver.FindParameter(step, "enabled")) {
            if (p->type == WorkflowParameterValue::Type::Number)
                grab = p->numberValue > 0.5;
            else if (p->type == WorkflowParameterValue::Type::Bool)
                grab = p->boolValue;
        }
    }

    SDL_SetWindowRelativeMouseMode(window, grab);
    context.Set<bool>("mouse_grabbed", grab);

    if (logger_) {
        logger_->Info("input.mouse.grab: " + std::string(grab ? "enabled" : "disabled"));
    }
}

}  // namespace sdl3cpp::services::impl
