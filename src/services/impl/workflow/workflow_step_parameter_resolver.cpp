#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStepParameterResolver::WorkflowStepParameterResolver(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "Constructor", "Entry");
    }
}

const WorkflowParameterValue* WorkflowStepParameterResolver::FindParameter(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "FindParameter", "Entry");
    }
    auto it = step.parameters.find(name);
    if (it == step.parameters.end()) {
        return nullptr;
    }
    return &it->second;
}

const WorkflowParameterValue& WorkflowStepParameterResolver::GetRequiredParameter(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredParameter", "Entry");
    }
    const auto* param = FindParameter(step, name);
    if (!param) {
        throw std::runtime_error("Workflow step '" + step.id + "' missing parameter '" + name + "'");
    }
    return *param;
}

std::string WorkflowStepParameterResolver::GetRequiredString(const WorkflowStepDefinition& step,
                                                             const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredString", "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::String) {
        throw std::runtime_error("Workflow step '" + step.id + "' parameter '" + name + "' must be a string");
    }
    return param.stringValue;
}

double WorkflowStepParameterResolver::GetRequiredNumber(const WorkflowStepDefinition& step,
                                                        const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredNumber", "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::Number) {
        throw std::runtime_error("Workflow step '" + step.id + "' parameter '" + name + "' must be a number");
    }
    return param.numberValue;
}

bool WorkflowStepParameterResolver::GetRequiredBool(const WorkflowStepDefinition& step,
                                                    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredBool", "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::Bool) {
        throw std::runtime_error("Workflow step '" + step.id + "' parameter '" + name + "' must be a bool");
    }
    return param.boolValue;
}

std::vector<std::string> WorkflowStepParameterResolver::GetRequiredStringList(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredStringList", "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::StringList) {
        throw std::runtime_error("Workflow step '" + step.id + "' parameter '" + name + "' must be string list");
    }
    return param.stringList;
}

std::vector<double> WorkflowStepParameterResolver::GetRequiredNumberList(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "GetRequiredNumberList", "Entry");
    }
    const auto& param = GetRequiredParameter(step, name);
    if (param.type != WorkflowParameterValue::Type::NumberList) {
        throw std::runtime_error("Workflow step '" + step.id + "' parameter '" + name + "' must be number list");
    }
    return param.numberList;
}

}  // namespace sdl3cpp::services::impl
