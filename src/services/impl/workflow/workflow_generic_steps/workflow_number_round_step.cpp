#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_round_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNumberRoundStep::WorkflowNumberRoundStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowNumberRoundStep::GetPluginId() const {
    return "number.round";
}

void WorkflowNumberRoundStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<double>(valueKey);
    if (!value) {
        throw std::runtime_error("number.round missing input '" + valueKey + "'");
    }

    const double result = std::round(*value);
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowNumberRoundStep", "Execute",
                       "value=" + std::to_string(*value) +
                           ", output=" + outputKey,
                       "Rounded workflow number");
    }
}

}  // namespace sdl3cpp::services::impl
