#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: racer.pod.drive
 *
 * Flies the player's pod one frame: reads input.move_forward,
 * input.move_right, racer.boost_pressed and racer.repair_pressed, steps
 * the pod physics over the track surface, and puts the pod back on the
 * lap if it falls off the course. Holds still during the countdown.
 * Parameter `autopilot` (e.g. "${env:RACER_AUTOPILOT}"): when set and
 * not "0", RacerAutopilot drives instead of the player.
 *
 * Publishes racer.pod_pos, racer.pod_heading, racer.speed (km/h as the
 * game shows it), racer.heat, racer.damage (and _left, _right) and
 * racer.boosting.
 */
class WorkflowRacerPodDriveStep final : public IWorkflowStep {
public:
    WorkflowRacerPodDriveStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    float traceClock_ = 0.f;
    RacerRecovery recovery_;
};

}  // namespace sdl3cpp::services::impl
