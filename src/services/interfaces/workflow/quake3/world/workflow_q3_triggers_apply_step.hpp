#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.triggers.apply
 *
 * Applies trigger effects computed by q3.triggers.check to game state.
 * Must run after q3.triggers.check in the same frame.
 *
 * Reads:  q3.player_velocity_override (glm::vec3, optional),
 *         q3.player_teleport_dest (glm::vec3, optional),
 *         q3.player_on_jump_pad (bool),
 *         q3.ps (nlohmann::json, optional Q3 player state),
 *         q3.player_pos (glm::vec3)
 * Writes: q3.ps (velocity/origin if present), q3.player_pos,
 *         clears q3.player_velocity_override, q3.player_teleport_dest,
 *         q3.player_on_jump_pad
 */
class WorkflowQ3TriggersApplyStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3TriggersApplyStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
