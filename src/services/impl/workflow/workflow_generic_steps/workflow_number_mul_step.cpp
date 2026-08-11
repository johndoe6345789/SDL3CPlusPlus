#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_mul_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNumberMulStep::WorkflowNumberMulStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowNumberMulStep::GetPluginId() const {
    return "number.mul";
}

void WorkflowNumberMulStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* left = context.TryGet<double>(leftKey);
    const auto* right = context.TryGet<double>(rightKey);
    if (!left || !right) {
        throw std::runtime_error("number.mul missing inputs '" + leftKey + "' or '" + rightKey + "'");
    }

    const double result = *left * *right;
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowNumberMulStep", "Execute",
                       "left=" + std::to_string(*left) +
                           ", right=" + std::to_string(*right) +
                           ", output=" + outputKey,
                       "Multiplied workflow numbers");
    }
}

}  // namespace sdl3cpp::services::impl
