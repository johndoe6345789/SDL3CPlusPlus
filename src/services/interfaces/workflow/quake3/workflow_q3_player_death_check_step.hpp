#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.player.death_check
 *
 * Detects when the player health first drops to zero or below.
 * Records the time of death and increments the death counter.
 * No-ops if already dead or health > 0.
 *
 * Context reads:
 *   q3.player_health   int
 *   q3.player_dead     bool
 *   frame.elapsed      double
 *
 * Context writes:
 *   q3.player_dead     bool  (set true on new death)
 *   q3.death_time      double
 *   q3.death_count     int   (incremented)
 */
class WorkflowQ3PlayerDeathCheckStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PlayerDeathCheckStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
