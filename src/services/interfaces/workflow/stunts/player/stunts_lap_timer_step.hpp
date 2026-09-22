#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: stunts.lap.timer
 *
 * Times laps against the cell the car started on. The clock starts
 * when the car first leaves that cell, so sitting on the line costs
 * nothing, and a lap counts once the car returns to it having been
 * away. Publishes stunts.lap, stunts.lap_time and stunts.best_lap.
 */
class WorkflowStuntsLapTimerStep final : public IWorkflowStep {
public:
    WorkflowStuntsLapTimerStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<StuntsWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    float lapTime_ = 0.f;
    float bestLap_ = 0.f;
    int lap_ = 0;
    bool running_ = false;
    bool away_ = false;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<StuntsWorldState> state_;
};

}  // namespace sdl3cpp::services::impl
