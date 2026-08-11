#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_clear_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowValueClearStep::WorkflowValueClearStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowValueClearStep::GetPluginId() const {
    return "value.clear";
}

void WorkflowValueClearStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string inputKey = resolver.GetRequiredInputKey(step, "value");
    const bool removed = context.Remove(inputKey);

    if (logger_) {
        logger_->Trace("WorkflowValueClearStep", "Execute",
                       "key=" + inputKey +
                           ", removed=" + std::string(removed ? "true" : "false"),
                       "Cleared workflow value");
    }
}

}  // namespace sdl3cpp::services::impl
