#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_not_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowBoolNotStep::WorkflowBoolNotStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBoolNotStep::GetPluginId() const {
    return "bool.not";
}

void WorkflowBoolNotStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<bool>(valueKey);
    if (!value) {
        throw std::runtime_error("bool.not requires bool input");
    }

    const bool result = !(*value);
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowBoolNotStep", "Execute",
                       "value=" + valueKey +
                           ", output=" + outputKey,
                       "Computed workflow NOT");
    }
}

}  // namespace sdl3cpp::services::impl
