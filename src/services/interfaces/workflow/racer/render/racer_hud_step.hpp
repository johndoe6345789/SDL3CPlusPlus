#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// The HUD line for a race state: the countdown, then lap, times, speed
/// and engine heat, then the result. Pure, so it is testable.
std::string FormatRacerHud(const RacerRaceState& race,
                           const RacerPodState& pod);

/**
 * Plugin ID: racer.hud.text
 *
 * Writes FormatRacerHud's line into the text overlay that overlay.fps_*
 * draws (in place of the FPS counter). Run it after
 * overlay.fps_upload_quad and before overlay.fps_draw.
 */
class WorkflowRacerHudStep final : public IWorkflowStep {
public:
    WorkflowRacerHudStep(std::shared_ptr<ILogger> logger,
                         std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    std::string shown_;
};

}  // namespace sdl3cpp::services::impl
