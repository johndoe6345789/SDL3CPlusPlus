#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pm.crouch
 *
 * Handles crouch/uncrouch transitions for the Q3 player movement system.
 * When crouching: shrinks bounding box (maxs.y → 0.85).
 * When uncrouching: traces upward first; only stands if there is room.
 * Also writes q3.player_pos, q3.player_yaw, q3.player_pitch for backward
 * compatibility with HUD and bot steps.
 *
 * Reads:  input.crouch (bool), q3.ps (Q3PlayerState),
 *         q3.player_yaw (float), q3.player_pitch (float),
 *         physics_world (btDiscreteDynamicsWorld*)
 * Writes: q3.ps, q3.player_pos (glm::vec3), q3.player_yaw (float),
 *         q3.player_pitch (float)
 */
class WorkflowQ3PmCrouchStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PmCrouchStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
