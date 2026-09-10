#include "services/interfaces/workflow/workflow_generic_steps/workflow_vfx_destroy_step.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/vfx_destroy_selection.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowVfxDestroyStep::WorkflowVfxDestroyStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowVfxDestroyStep::GetPluginId() const {
    return "vfx.destroy";
}

void WorkflowVfxDestroyStep::Execute(const WorkflowStepDefinition& step,
                                     WorkflowContext& context) {
    WorkflowStepParameterResolver parameterResolver;

    std::vector<std::string> effects;
    if (const auto* existingEffects =
            context.TryGet<std::vector<std::string>>("vfx.active")) {
        effects = *existingEffects;
    }

    const bool destroyed =
        ApplyVfxDestroySelection(step, parameterResolver, effects);

    context.Set("vfx.active", effects);

    const auto destroyedOutputIt = step.outputs.find("destroyed");
    if (destroyedOutputIt != step.outputs.end()) {
        context.Set(destroyedOutputIt->second, destroyed);
    }

    const auto countOutputIt = step.outputs.find("remaining_count");
    if (countOutputIt != step.outputs.end()) {
        context.Set(countOutputIt->second, static_cast<double>(effects.size()));
    }

    if (logger_) {
        logger_->Trace(
            "WorkflowVfxDestroyStep", "Execute",
            "destroyed=" + std::string(destroyed ? "true" : "false") +
                ", remaining=" + std::to_string(effects.size()),
            "VFX destruction complete");
    }
}

}  // namespace sdl3cpp::services::impl
