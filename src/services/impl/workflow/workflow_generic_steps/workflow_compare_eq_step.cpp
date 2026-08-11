#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_eq_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowCompareEqStep::WorkflowCompareEqStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCompareEqStep::GetPluginId() const {
    return "compare.eq";
}

void WorkflowCompareEqStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    bool result = false;
    if (const auto* left = context.TryGet<double>(leftKey)) {
        const auto* right = context.TryGet<double>(rightKey);
        if (!right) {
            throw std::runtime_error("compare.eq requires both inputs to be numbers");
        }
        result = (*left == *right);
    } else if (const auto* left = context.TryGet<std::string>(leftKey)) {
        const auto* right = context.TryGet<std::string>(rightKey);
        if (!right) {
            throw std::runtime_error("compare.eq requires both inputs to be strings");
        }
        result = (*left == *right);
    } else if (const auto* left = context.TryGet<bool>(leftKey)) {
        const auto* right = context.TryGet<bool>(rightKey);
        if (!right) {
            throw std::runtime_error("compare.eq requires both inputs to be bools");
        }
        result = (*left == *right);
    } else {
        throw std::runtime_error("compare.eq requires number, string, or bool inputs");
    }

    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowCompareEqStep", "Execute",
                       "left=" + leftKey +
                           ", right=" + rightKey +
                           ", output=" + outputKey,
                       "Compared workflow values for equality");
    }
}

}  // namespace sdl3cpp::services::impl
