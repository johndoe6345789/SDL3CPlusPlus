#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_abs_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNumberAbsStep::WorkflowNumberAbsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowNumberAbsStep::GetPluginId() const {
    return "number.abs";
}

void WorkflowNumberAbsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<double>(valueKey);
    if (!value) {
        throw std::runtime_error("number.abs missing input '" + valueKey + "'");
    }

    const double result = std::abs(*value);
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowNumberAbsStep", "Execute",
                       "value=" + std::to_string(*value) +
                           ", output=" + outputKey,
                       "Computed workflow absolute value");
    }
}

}  // namespace sdl3cpp::services::impl
