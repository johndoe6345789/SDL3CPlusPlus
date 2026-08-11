#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_concat_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStringConcatStep::WorkflowStringConcatStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringConcatStep::GetPluginId() const {
    return "string.concat";
}

void WorkflowStringConcatStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* left = context.TryGet<std::string>(leftKey);
    const auto* right = context.TryGet<std::string>(rightKey);
    if (!left || !right) {
        throw std::runtime_error("string.concat requires string inputs");
    }

    context.Set(outputKey, *left + *right);

    if (logger_) {
        logger_->Trace("WorkflowStringConcatStep", "Execute",
                       "left=" + leftKey +
                           ", right=" + rightKey +
                           ", output=" + outputKey,
                       "Concatenated workflow strings");
    }
}

}  // namespace sdl3cpp::services::impl
