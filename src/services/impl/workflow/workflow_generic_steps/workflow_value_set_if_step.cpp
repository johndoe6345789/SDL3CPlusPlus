#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_set_if_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowValueSetIfStep::WorkflowValueSetIfStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowValueSetIfStep::GetPluginId() const {
    return "value.set_if";
}

void WorkflowValueSetIfStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;
    WorkflowStepParameterResolver paramResolver;

    const std::string condKey = ioResolver.GetRequiredInputKey(step, "condition");
    const auto* cond = context.TryGet<bool>(condKey);
    if (!cond) {
        throw std::runtime_error("value.set_if: condition key '" + condKey + "' is not a bool");
    }

    if (!*cond) return;

    const std::string outputKey = ioResolver.GetRequiredOutputKey(step, "value");
    const auto& param = paramResolver.GetRequiredParameter(step, "value");

    switch (param.type) {
    case WorkflowParameterValue::Type::Bool:
        context.Set(outputKey, param.boolValue);
        break;
    case WorkflowParameterValue::Type::Number:
        context.Set(outputKey, param.numberValue);
        break;
    case WorkflowParameterValue::Type::String:
        context.Set(outputKey, param.stringValue);
        break;
    default:
        throw std::runtime_error("value.set_if: unsupported parameter type");
    }

    if (logger_) {
        logger_->Trace("WorkflowValueSetIfStep", "Execute",
                       "condition=" + condKey + ", output=" + outputKey,
                       "Conditionally set workflow value");
    }
}

}  // namespace sdl3cpp::services::impl
