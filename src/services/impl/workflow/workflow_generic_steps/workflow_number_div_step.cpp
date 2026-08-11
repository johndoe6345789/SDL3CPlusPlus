#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_div_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNumberDivStep::WorkflowNumberDivStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowNumberDivStep::GetPluginId() const {
    return "number.div";
}

void WorkflowNumberDivStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* left = context.TryGet<double>(leftKey);
    const auto* right = context.TryGet<double>(rightKey);
    if (!left || !right) {
        throw std::runtime_error("number.div missing inputs '" + leftKey + "' or '" + rightKey + "'");
    }
    if (*right == 0.0) {
        throw std::runtime_error("number.div divide by zero");
    }

    const double result = *left / *right;
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowNumberDivStep", "Execute",
                       "left=" + std::to_string(*left) +
                           ", right=" + std::to_string(*right) +
                           ", output=" + outputKey,
                       "Divided workflow numbers");
    }
}

}  // namespace sdl3cpp::services::impl
