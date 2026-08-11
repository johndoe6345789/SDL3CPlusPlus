#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_equals_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStringEqualsStep::WorkflowStringEqualsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringEqualsStep::GetPluginId() const {
    return "string.equals";
}

void WorkflowStringEqualsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* left = context.TryGet<std::string>(leftKey);
    const auto* right = context.TryGet<std::string>(rightKey);
    if (!left || !right) {
        throw std::runtime_error("string.equals requires string inputs");
    }

    const bool result = (*left == *right);
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowStringEqualsStep", "Execute",
                       "left=" + leftKey +
                           ", right=" + rightKey +
                           ", output=" + outputKey,
                       "Compared workflow strings for equality");
    }
}

}  // namespace sdl3cpp::services::impl
