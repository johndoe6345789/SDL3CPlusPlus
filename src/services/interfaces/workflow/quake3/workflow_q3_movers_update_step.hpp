#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.movers.update
 *
 * Per-frame mover simulation: doors/platforms move between pos1 and pos2.
 * Proximity to player triggers door open. Pushes player if mover overlaps.
 *
 * Reads:  q3.movers (MoverList), q3.player_pos (glm::vec3), frame.delta_time
 * Writes: q3.movers (in place), q3.player_velocity_push (glm::vec3, additive)
 */
class WorkflowQ3MoversUpdateStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3MoversUpdateStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
