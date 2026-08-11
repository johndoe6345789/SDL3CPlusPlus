#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pm.accelerate
 *
 * Applies wish-direction acceleration, matching bg_pmove.c PM_Accelerate.
 * Builds a wish direction from input axes rotated by player yaw, then
 * adds velocity up to a maximum (wishspeed) using the Q3 accel formula.
 *
 * Reads:  q3.ps (Q3PlayerState),
 *         input.move_forward (float -1..1),
 *         input.move_right   (float -1..1),
 *         q3.player_yaw      (float, radians),
 *         frame.delta_time   (double)
 * Writes: q3.ps
 */
class WorkflowQ3PmAccelerateStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PmAccelerateStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
