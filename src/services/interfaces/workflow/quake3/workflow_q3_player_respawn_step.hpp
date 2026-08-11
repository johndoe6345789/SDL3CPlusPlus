#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.player.respawn
 *
 * Respawns the player 1.7 seconds after death.
 * Picks a random BSP spawn point (bsp.spawn_points), resets health/armor/ammo.
 * Clears q3.ammo_initialized so q3.ammo.init re-runs next frame.
 *
 * Context reads:
 *   q3.player_dead      bool
 *   q3.death_time       double
 *   frame.elapsed       double
 *   bsp.spawn_points    nlohmann::json array of {x,y,z}
 *
 * Context writes:
 *   q3.player_pos       glm::vec3
 *   q3.player_health    int
 *   q3.player_armor     int
 *   q3.armor_type       std::string
 *   q3.player_dead      bool (false)
 *   q3.ammo_initialized bool (false — triggers re-init)
 */
class WorkflowQ3PlayerRespawnStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PlayerRespawnStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
