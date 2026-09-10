#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

std::vector<std::string>
WorkflowStepParameterResolver::GetRequiredStringList(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver",
                       "GetRequiredStringList", "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::StringList) {
        throw std::runtime_error("Workflow step '" + step.id +
                                 "' parameter '" + name +
                                 "' must be string list");
    }
    return param.stringList;
}

std::vector<double> WorkflowStepParameterResolver::GetRequiredNumberList(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver",
                       "GetRequiredNumberList", "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::NumberList) {
        throw std::runtime_error("Workflow step '" + step.id +
                                 "' parameter '" + name +
                                 "' must be number list");
    }
    return param.numberList;
}

}  // namespace sdl3cpp::services::impl
