#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_clamp_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowNumberClampStep::WorkflowNumberClampStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowNumberClampStep::GetPluginId() const {
    return "number.clamp";
}

void WorkflowNumberClampStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string minKey = resolver.GetRequiredInputKey(step, "min");
    const std::string maxKey = resolver.GetRequiredInputKey(step, "max");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<double>(valueKey);
    const auto* minValue = context.TryGet<double>(minKey);
    const auto* maxValue = context.TryGet<double>(maxKey);
    if (!value || !minValue || !maxValue) {
        throw std::runtime_error("number.clamp missing inputs '" + valueKey + "', '" +
                                 minKey + "', or '" + maxKey + "'");
    }

    const double clamped = std::clamp(*value, *minValue, *maxValue);
    context.Set(outputKey, clamped);

    if (logger_) {
        logger_->Trace("WorkflowNumberClampStep", "Execute",
                       "value=" + std::to_string(*value) +
                           ", min=" + std::to_string(*minValue) +
                           ", max=" + std::to_string(*maxValue) +
                           ", output=" + outputKey,
                       "Clamped workflow number");
    }
}

}  // namespace sdl3cpp::services::impl
