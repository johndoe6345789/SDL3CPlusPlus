#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.menu
 *
 * Moves the session between the main menu, the race and the finish screen
 * from the keys: Up and Down move the cursor, Enter picks, Escape goes back.
 *
 * Reads:  input_key_{up,down,enter,escape}_pressed, switchback.race.finished
 * Writes: the shared SwitchbackSession
 */
class WorkflowSwitchbackMenuStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackMenuStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<SwitchbackSession> session);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<SwitchbackSession> session_;
};

}  // namespace sdl3cpp::services::impl
