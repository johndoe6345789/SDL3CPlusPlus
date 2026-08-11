#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_copy_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowValueCopyStep::WorkflowValueCopyStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowValueCopyStep::GetPluginId() const {
    return "value.copy";
}

void WorkflowValueCopyStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string inputKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGetAny(inputKey);
    if (!value) {
        throw std::runtime_error("value.copy missing input '" + inputKey + "'");
    }

    context.Set(outputKey, *value);

    if (logger_) {
        logger_->Trace("WorkflowValueCopyStep", "Execute",
                       "input=" + inputKey + ", output=" + outputKey,
                       "Copied workflow value");
    }
}

}  // namespace sdl3cpp::services::impl
