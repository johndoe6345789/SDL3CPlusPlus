#include "services/interfaces/workflow/workflow_step_registry.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStepRegistry::WorkflowStepRegistry(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowStepRegistry", "Constructor", "Entry");
    }
}

void WorkflowStepRegistry::RegisterStep(std::shared_ptr<IWorkflowStep> step) {
    if (logger_) {
        logger_->Trace("WorkflowStepRegistry", "RegisterStep", "Entry");
    }
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
    if (logger_) {
        logger_->Trace("WorkflowStepRegistry", "GetStep", "Entry");
    }
    auto it = steps_.find(pluginId);
    if (it == steps_.end()) {
        return nullptr;
    }
    return it->second;
}

}  // namespace sdl3cpp::services::impl
