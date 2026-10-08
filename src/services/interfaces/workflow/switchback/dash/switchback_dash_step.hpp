#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.dash
 *
 * The main menu while the session is on the menu screen. Otherwise the MPH
 * and RPM dials over the finished frame, unless the dashboard is switched
 * off. After the composite, before the command buffer submit.
 *
 * Parameters: minimap_dir (optional; the dials do not need it)
 * Reads:      switchback.route.active, switchback.checkpoint.passed/total,
 *             switchback.race.finished
 */
class WorkflowSwitchbackDashStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackDashStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<Gta5StreamState> state,
                               std::shared_ptr<SwitchbackSession> session);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    std::shared_ptr<SwitchbackSession> session_;
    Gta5Hud hud_;
};

}  // namespace sdl3cpp::services::impl
