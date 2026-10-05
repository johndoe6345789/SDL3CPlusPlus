#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: racer.lap.timer
 *
 * Runs the start countdown, then tracks the pod along the lap and times
 * each lap. Publishes racer.lap, racer.laps, racer.race_time,
 * racer.lap_time, racer.best_lap, racer.countdown and racer.finished.
 */
class WorkflowRacerLapTimerStep final : public IWorkflowStep {
public:
    WorkflowRacerLapTimerStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
};

}  // namespace sdl3cpp::services::impl
