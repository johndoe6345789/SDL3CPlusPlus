#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.triggers.check
 *
 * One-time parse of trigger entities on first call, then per-frame overlap tests.
 * Handles: trigger_push (jump pad), trigger_teleport, trigger_hurt.
 *
 * Reads:  bsp.entities (nlohmann::json array), q3.player_pos (glm::vec3), q3.player_dead (bool)
 * Writes: q3.player_velocity_override (glm::vec3), q3.player_on_jump_pad (bool),
 *         q3.player_teleport_dest (glm::vec3), q3.pending_damage (float, additive)
 */
class WorkflowQ3TriggersCheckStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3TriggersCheckStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
