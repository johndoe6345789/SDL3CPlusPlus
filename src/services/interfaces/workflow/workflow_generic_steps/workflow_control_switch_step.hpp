#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: control.condition.switch
 *
 * Stringifies the "value" input (string/bool/double/int) and dispatches
 * to whichever "case_<value>" input names a registered step id, falling
 * back to "default" when no case matches. See switch_value_matcher.hpp
 * for the stringification/case-matching logic.
 */
class WorkflowControlSwitchStep final : public IWorkflowStep {
public:
    explicit WorkflowControlSwitchStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IWorkflowStepRegistry> registry);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWorkflowStepRegistry> registry_;
};

}  // namespace sdl3cpp::services::impl
