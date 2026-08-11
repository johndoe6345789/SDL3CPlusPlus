#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pm.slide_move
 *
 * Integrates the player velocity against the Bullet world using Q3's
 * iterative slide-move algorithm (bg_slidemove.c PM_SlideMove).
 *
 * Up to 4 collision bumps per frame:
 *   1. Sweep box from origin in the direction of velocity * timeLeft.
 *   2. Advance origin to hit point.
 *   3. Clip velocity against the hit normal (overbounce 1.001).
 *   4. If the clipped velocity would re-enter a previously hit plane,
 *      project along the crease (cross product of both normals).
 *
 * After integration, syncs q3.player_pos ← ps.origin.
 *
 * Reads:  q3.ps (Q3PlayerState),
 *         physics_world (btDiscreteDynamicsWorld*),
 *         frame.delta_time (double)
 * Writes: q3.ps, q3.player_pos (glm::vec3)
 */
class WorkflowQ3PmSlideMoveStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PmSlideMoveStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
