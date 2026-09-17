#pragma once

#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/i_logger.hpp"

namespace sdl3cpp::services::impl {

class WorkflowExecutor : public IWorkflowExecutor {
public:
    WorkflowExecutor(std::shared_ptr<IWorkflowStepRegistry> registry,
                     std::shared_ptr<ILogger> logger);

    void Execute(const WorkflowDefinition& workflow, WorkflowContext& context) override;

private:
    std::shared_ptr<IWorkflowStepRegistry> registry_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
