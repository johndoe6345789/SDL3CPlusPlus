#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string WorkflowStepParameterResolver::GetRequiredString(
    const WorkflowStepDefinition& step, const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredString",
                       "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::String) {
        throw std::runtime_error("Workflow step '" + step.id +
                                 "' parameter '" + name +
                                 "' must be a string");
    }
    return param.stringValue;
}

double WorkflowStepParameterResolver::GetRequiredNumber(
    const WorkflowStepDefinition& step, const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredNumber",
                       "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::Number) {
        throw std::runtime_error("Workflow step '" + step.id +
                                 "' parameter '" + name +
                                 "' must be a number");
    }
    return param.numberValue;
}

bool WorkflowStepParameterResolver::GetRequiredBool(
    const WorkflowStepDefinition& step, const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredBool",
                       "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::Bool) {
        throw std::runtime_error("Workflow step '" + step.id +
                                 "' parameter '" + name + "' must be a bool");
    }
    return param.boolValue;
}

}  // namespace sdl3cpp::services::impl
