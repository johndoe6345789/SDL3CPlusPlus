#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: control.try.catch
 *
 * Executes "try_step"; on exception, stores the message under
 * "error_output" (default "error.message") and runs "catch_step" if one
 * was given, re-throwing if the catch step itself throws. See
 * try_catch_dispatch.hpp for the shared step-dispatch helper.
 */
class WorkflowTryCatchStep final : public IWorkflowStep {
public:
    WorkflowTryCatchStep(std::shared_ptr<ILogger> logger,
                         std::shared_ptr<IWorkflowStepRegistry> registry);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWorkflowStepRegistry> registry_;
};

}  // namespace sdl3cpp::services::impl
