#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.damage.apply
 *
 * Applies pending player damage with armor absorption (ioq3 G_Damage logic).
 * Returns early if q3.pending_damage == 0.
 *
 * Context reads:
 *   q3.pending_damage   int
 *   q3.player_health    int
 *   q3.player_armor     int
 *   q3.armor_type       std::string ("none" | "green" | "yellow")
 *
 * Context writes:
 *   q3.player_health    int
 *   q3.player_armor     int
 *   q3.pending_damage   int (cleared to 0)
 */
class WorkflowQ3DamageApplyStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3DamageApplyStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
