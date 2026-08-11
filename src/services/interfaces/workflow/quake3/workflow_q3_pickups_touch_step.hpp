#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pickups.touch
 *
 * AABB proximity test between the player and each uncollected BSP pickup entity.
 * Applies health, armor, ammo, and weapon pickups on contact.
 * Records collection time for respawn timer.
 *
 * Context reads:
 *   q3.player_pos           glm::vec3
 *   q3.player_health        int
 *   q3.player_armor         int
 *   q3.armor_type           std::string
 *   q3.player_ammo          nlohmann::json object
 *   q3.player_dead          bool
 *   q3.collected            nlohmann::json object  {entityId: bool}
 *   bsp.entities            nlohmann::json array
 *   frame.elapsed           double
 *
 * Context writes:
 *   q3.player_health        int
 *   q3.player_armor         int
 *   q3.armor_type           std::string
 *   q3.player_ammo          nlohmann::json object
 *   q3.inventory            nlohmann::json object  {classname: bool}
 *   q3.collected            nlohmann::json object
 *   q3.pickup_respawn_times nlohmann::json object  {entityId: double}
 */
class WorkflowQ3PickupsTouchStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PickupsTouchStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
