#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStepIoResolver::WorkflowStepIoResolver(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowStepIoResolver", "Constructor", "Entry");
    }
}

std::string WorkflowStepIoResolver::GetRequiredInputKey(const WorkflowStepDefinition& step,
                                                        const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepIoResolver", "GetRequiredInputKey", "Entry");
    }
    auto it = step.inputs.find(name);
    if (it == step.inputs.end()) {
        throw std::runtime_error("Workflow step '" + step.id + "' missing input '" + name + "'");
    }
    return it->second;
}

std::string WorkflowStepIoResolver::GetRequiredOutputKey(const WorkflowStepDefinition& step,
                                                         const std::string& name) const {
    if (logger_) {
        logger_->Trace("WorkflowStepIoResolver", "GetRequiredOutputKey", "Entry");
    }
    auto it = step.outputs.find(name);
    if (it == step.outputs.end()) {
        throw std::runtime_error("Workflow step '" + step.id + "' missing output '" + name + "'");
    }
    return it->second;
}

std::string WorkflowStepIoResolver::GetOptionalParameterValue(const WorkflowStepDefinition& step,
                                                              const std::string& name,
                                                              const std::string& defaultValue) const {
    if (logger_) {
        logger_->Trace("WorkflowStepIoResolver", "GetOptionalParameterValue", "Entry");
    }
    auto it = step.parameters.find(name);
    if (it == step.parameters.end()) {
        return defaultValue;
    }
    return it->second.stringValue;
}

}  // namespace sdl3cpp::services::impl
