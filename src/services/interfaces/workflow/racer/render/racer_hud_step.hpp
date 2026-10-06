#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/render/racer_panel.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// The HUD lines for a race state: the countdown, then lap, times, speed
/// and engine heat, then the result. Pure, so it is testable.
std::string FormatRacerHud(const RacerRaceState& race,
                           const RacerPodState& pod);

/**
 * Plugin ID: racer.hud.text
 *
 * Draws FormatRacerHud's four lines in the top-right corner, at three
 * times the size of the engine's FPS overlay, whose pipeline it borrows
 * (run overlay.fps_init first). Run after frame.gpu.end_scene.
 */
class WorkflowRacerHudStep final : public IWorkflowStep {
public:
    WorkflowRacerHudStep(std::shared_ptr<ILogger> logger,
                         std::shared_ptr<RacerWorldState> state);
    ~WorkflowRacerHudStep() override;
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    std::string shown_;
    RacerPanel hud_;
};

}  // namespace sdl3cpp::services::impl
