#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_or_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowBoolOrStep::WorkflowBoolOrStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBoolOrStep::GetPluginId() const {
    return "bool.or";
}

void WorkflowBoolOrStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* left = context.TryGet<bool>(leftKey);
    const auto* right = context.TryGet<bool>(rightKey);
    if (!left || !right) {
        throw std::runtime_error("bool.or requires bool inputs");
    }

    const bool result = *left || *right;
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowBoolOrStep", "Execute",
                       "left=" + leftKey +
                           ", right=" + rightKey +
                           ", output=" + outputKey,
                       "Computed workflow OR");
    }
}

}  // namespace sdl3cpp::services::impl
