#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.stats.init
 *
 * Gives the player their starting health and armour. Damage, pickups
 * and respawn all write these keys, but nothing set them at map load,
 * so the HUD read a default that never changed.
 *
 * ioq3 spawns with STAT_MAX_HEALTH + 25 (g_client.c ClientSpawn), which
 * is the 125 that decays back to 100.
 *
 * Writes: q3.player_health, q3.player_armor, q3.player_max_health
 */
class WorkflowQ3StatsInitStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3StatsInitStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
