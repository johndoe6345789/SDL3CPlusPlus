#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowControlSwitchStep final : public IWorkflowStep {
public:
    explicit WorkflowControlSwitchStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IWorkflowStepRegistry> registry);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWorkflowStepRegistry> registry_;
};

}  // namespace sdl3cpp::services::impl
