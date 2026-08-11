#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_add_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNumberAddStep::WorkflowNumberAddStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowNumberAddStep::GetPluginId() const {
    return "number.add";
}

void WorkflowNumberAddStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* left = context.TryGet<double>(leftKey);
    const auto* right = context.TryGet<double>(rightKey);
    if (!left || !right) {
        throw std::runtime_error("number.add missing inputs '" + leftKey + "' or '" + rightKey + "'");
    }

    const double sum = *left + *right;
    context.Set(outputKey, sum);

    if (logger_) {
        logger_->Trace("WorkflowNumberAddStep", "Execute",
                       "left=" + std::to_string(*left) +
                           ", right=" + std::to_string(*right) +
                           ", output=" + outputKey,
                       "Added workflow numbers");
    }
}

}  // namespace sdl3cpp::services::impl
