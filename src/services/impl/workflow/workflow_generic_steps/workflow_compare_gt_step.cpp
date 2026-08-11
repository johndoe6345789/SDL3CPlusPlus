#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_gt_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowCompareGtStep::WorkflowCompareGtStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCompareGtStep::GetPluginId() const {
    return "compare.gt";
}

void WorkflowCompareGtStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* left = context.TryGet<double>(leftKey);
    const auto* right = context.TryGet<double>(rightKey);
    if (!left || !right) {
        throw std::runtime_error("compare.gt requires number inputs");
    }

    const bool result = *left > *right;
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowCompareGtStep", "Execute",
                       "left=" + leftKey +
                           ", right=" + rightKey +
                           ", output=" + outputKey,
                       "Compared workflow values (>)");
    }
}

}  // namespace sdl3cpp::services::impl
