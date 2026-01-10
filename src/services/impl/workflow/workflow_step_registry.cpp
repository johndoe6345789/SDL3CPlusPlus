#include "workflow_step_registry.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

void WorkflowStepRegistry::RegisterStep(std::shared_ptr<IWorkflowStep> step) {
    if (!step) {
        throw std::runtime_error("WorkflowStepRegistry::RegisterStep: step is null");
    }
    const std::string pluginId = step->GetPluginId();
    auto [it, inserted] = steps_.emplace(pluginId, std::move(step));
    if (!inserted) {
        throw std::runtime_error("WorkflowStepRegistry::RegisterStep: duplicate plugin '" + pluginId + "'");
    }
}

std::shared_ptr<IWorkflowStep> WorkflowStepRegistry::GetStep(const std::string& pluginId) const {
    auto it = steps_.find(pluginId);
    if (it == steps_.end()) {
        return nullptr;
    }
    return it->second;
}

}  // namespace sdl3cpp::services::impl
