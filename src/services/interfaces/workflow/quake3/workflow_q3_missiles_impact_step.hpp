#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <unordered_map>
#include <cstdint>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.missiles.impact
 *
 * Detects missile-world collisions using Bullet raycasts from each missile's
 * previous position to its current position. On impact:
 *   - Marks the missile as exploded.
 *   - Calculates splash damage to the player and each bot within splashRadius.
 *   - Accumulates damage into q3.pending_damage (player) and q3.bot_damage (bots).
 *   - Removes all exploded missiles from the vector.
 *
 * Splash damage formula (ioq3): dmg = splashDamage * (1 - dist / splashRadius)
 *
 * Reads:
 *   q3.missiles   (MissileList)
 *   physics_world (btDiscreteDynamicsWorld*)
 *   q3.player_pos (glm::vec3)
 *   q3.bots       (nlohmann::json)
 *   q3.pending_damage (int)
 * Writes:
 *   q3.missiles, q3.pending_damage, q3.bot_damage (nlohmann::json)
 */
class WorkflowQ3MissilesImpactStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3MissilesImpactStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    // Tracks the previous-frame origin of each missile by ID.
    std::unordered_map<uint32_t, glm::vec3> prevPositions_;
};

}  // namespace sdl3cpp::services::impl
