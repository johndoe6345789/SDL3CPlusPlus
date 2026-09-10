#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_definition.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: control.loop.while
 *
 * Repeatedly executes a sub-workflow (loaded via LoadPackageWorkflow, see
 * workflow_package_loader.hpp) while a context bool stays true, up to an
 * optional max_iterations safety valve.
 */
class WorkflowControlWhileStep final : public IWorkflowStep {
public:
    WorkflowControlWhileStep(std::shared_ptr<ILogger> logger,
                             std::shared_ptr<IWorkflowExecutor> executor);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWorkflowExecutor> executor_;
};

}  // namespace sdl3cpp::services::impl
