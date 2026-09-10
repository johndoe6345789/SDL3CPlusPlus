#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStepParameterResolver::WorkflowStepParameterResolver(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "Constructor",
                       "Entry");
    }
}

const WorkflowParameterValue* WorkflowStepParameterResolver::FindParameter(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver", "FindParameter",
                       "Entry");
    }
    auto it = step.parameters.find(name);
    if (it == step.parameters.end()) {
        return nullptr;
    }
    return &it->second;
}

const WorkflowParameterValue&
WorkflowStepParameterResolver::GetRequiredParameter(
    const WorkflowStepDefinition& step,
    const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepParameterResolver",
                       "GetRequiredParameter", "Entry");
    }
    const auto* param = FindParameter(step, name);
    if (!param) {
        throw std::runtime_error("Workflow step '" + step.id +
                                 "' missing parameter '" + name + "'");
    }
    return *param;
}

}  // namespace sdl3cpp::services::impl
