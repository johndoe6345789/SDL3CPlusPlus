#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.dash
 *
 * The MPH and RPM dials over the finished frame, while the player is in
 * the car. After the composite, before the command buffer submit.
 *
 * Parameters: minimap_dir (optional; the dials do not need it)
 */
class WorkflowSwitchbackDashStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackDashStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5Hud hud_;
};

}  // namespace sdl3cpp::services::impl
