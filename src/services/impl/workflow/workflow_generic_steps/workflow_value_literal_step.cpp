#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_literal_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowValueLiteralStep::WorkflowValueLiteralStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowValueLiteralStep::GetPluginId() const {
    return "value.literal";
}

void WorkflowValueLiteralStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string outputKey = ioResolver.GetRequiredOutputKey(step, "value");
    const auto& parameter = parameterResolver.GetRequiredParameter(step, "value");

    switch (parameter.type) {
    case WorkflowParameterValue::Type::String:
        context.Set(outputKey, parameter.stringValue);
        break;
    case WorkflowParameterValue::Type::Number:
        context.Set(outputKey, parameter.numberValue);
        break;
    case WorkflowParameterValue::Type::Bool:
        context.Set(outputKey, parameter.boolValue);
        break;
    case WorkflowParameterValue::Type::StringList:
        context.Set(outputKey, parameter.stringList);
        break;
    case WorkflowParameterValue::Type::NumberList:
        context.Set(outputKey, parameter.numberList);
        break;
    default:
        throw std::runtime_error("value.literal parameter type unsupported");
    }

    if (logger_) {
        logger_->Trace("WorkflowValueLiteralStep", "Execute",
                       "output=" + outputKey,
                       "Set literal workflow value");
    }
}

}  // namespace sdl3cpp::services::impl
