#include "workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string WorkflowStepIoResolver::GetRequiredInputKey(const WorkflowStepDefinition& step,
                                                        const std::string& name) const {
    auto it = step.inputs.find(name);
    if (it == step.inputs.end()) {
        throw std::runtime_error("Workflow step '" + step.id + "' missing input '" + name + "'");
    }
    return it->second;
}

std::string WorkflowStepIoResolver::GetRequiredOutputKey(const WorkflowStepDefinition& step,
                                                         const std::string& name) const {
    auto it = step.outputs.find(name);
    if (it == step.outputs.end()) {
        throw std::runtime_error("Workflow step '" + step.id + "' missing output '" + name + "'");
    }
    return it->second;
}

}  // namespace sdl3cpp::services::impl
