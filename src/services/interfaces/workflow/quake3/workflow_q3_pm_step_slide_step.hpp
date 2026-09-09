#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pm.step_slide
 *
 * Moves the player, and when the move is obstructed retries it from one
 * step height higher so stairs, ledges and doorway lips are walked over
 * rather than blocking. Mirrors ioq3 bg_slidemove.c PM_StepSlideMove.
 *
 * Replaces q3.pm.slide_move in the frame: a plain slide cannot climb, so
 * without this every step in a map is a wall.
 *
 * Reads:  q3.ps, physics_world, frame.delta_time
 * Writes: q3.ps, q3.player_pos, q3.step_delta
 */
class WorkflowQ3PmStepSlideStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PmStepSlideStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
