#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <nlohmann/json.hpp>

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.menu.update
 *
 * Drives the in-game menu state machine each frame: Escape toggles
 * open/close (or navigates back on a sub-screen), Up/Down move the
 * selection, and Enter dispatches the selected item's action. See
 * q3_menu_navigation.hpp for the toggle/input/item-building logic.
 */
class WorkflowQ3MenuUpdateStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3MenuUpdateStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    nlohmann::json config_;
    bool config_loaded_ = false;
};

}  // namespace sdl3cpp::services::impl
