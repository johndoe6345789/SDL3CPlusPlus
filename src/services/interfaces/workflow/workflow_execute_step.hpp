#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/workflow_definition.hpp"
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow composition step that executes a child workflow.
 *
 * Allows workflows to call other workflows like functions, enabling
 * proper OO-style composition and code reuse.
 *
 * Example:
 * {
 *   "type": "workflow.execute",
 *   "parameters": {
 *     "package": "seed",
 *     "workflow": "game_setup"
 *   }
 * }
 */
class WorkflowExecuteStep : public IWorkflowStep {
public:
    WorkflowExecuteStep(std::shared_ptr<ILogger> logger,
                        std::shared_ptr<IWorkflowExecutor> executor);
    ~WorkflowExecuteStep() override = default;

    std::string GetPluginId() const override { return "workflow.execute"; }
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWorkflowExecutor> executor_;

    WorkflowDefinition LoadWorkflow(const std::string& package, const std::string& workflowName);
};

}  // namespace sdl3cpp::services::impl
