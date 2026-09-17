#pragma once

#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

class WorkflowStepRegistry : public IWorkflowStepRegistry {
public:
    explicit WorkflowStepRegistry(std::shared_ptr<ILogger> logger = nullptr);

    void RegisterStep(std::shared_ptr<IWorkflowStep> step) override;
    std::shared_ptr<IWorkflowStep> GetStep(const std::string& pluginId) const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::unordered_map<std::string, std::shared_ptr<IWorkflowStep>> steps_;
};

}  // namespace sdl3cpp::services::impl
