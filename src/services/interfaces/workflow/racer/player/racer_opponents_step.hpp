#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Flies one opponent a frame: lap progress, autopilot, physics, and
/// putting them back on the course when lost.
void FlyRacerOpponent(RacerWorldState& state, RacerOpponent& opponent,
                      float dt);

/**
 * Plugin ID: racer.opponents.fly
 *
 * Flies every opponent, keeps pods from passing through each other, and
 * ranks the field. Run it after racer.pod.drive and racer.lap.timer.
 * Publishes racer.position and racer.entrants.
 */
class WorkflowRacerOpponentsStep final : public IWorkflowStep {
public:
    WorkflowRacerOpponentsStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
};

}  // namespace sdl3cpp::services::impl
