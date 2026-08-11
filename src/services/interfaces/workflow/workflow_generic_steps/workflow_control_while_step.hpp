#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_definition.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

class WorkflowControlWhileStep final : public IWorkflowStep {
public:
    WorkflowControlWhileStep(std::shared_ptr<ILogger> logger,
                             std::shared_ptr<IWorkflowExecutor> executor);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWorkflowExecutor> executor_;

    WorkflowDefinition LoadWorkflow(const std::string& package, const std::string& workflowName);
};

}  // namespace sdl3cpp::services::impl
