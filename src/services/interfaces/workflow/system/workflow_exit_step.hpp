#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Exit/quit the application with optional status code.
 *
 * Atomic step for cleanly terminating the application from workflow.
 * Useful for testing/validation workflows that need to exit after completion.
 *
 * Parameters:
 *   status_code: Exit code (default 0)
 *   message: Optional exit message to log
 */
class WorkflowExitStep final : public IWorkflowStep {
public:
    explicit WorkflowExitStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
