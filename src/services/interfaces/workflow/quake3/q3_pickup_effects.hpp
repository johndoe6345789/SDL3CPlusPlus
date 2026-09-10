#pragma once

#include "services/interfaces/i_logger.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Player-facing state a pickup touch reads and mutates. Round-trips
/// through WorkflowContext keys of the same shape in
/// WorkflowQ3PickupsTouchStep::Execute.
struct PickupTouchState {
    int health = 100;
    int armor  = 0;
    std::string armorType = "none";
    nlohmann::json ammo         = nlohmann::json::object();
    nlohmann::json inventory    = nlohmann::json::object();
    nlohmann::json collected    = nlohmann::json::object();
    nlohmann::json respawnTimes = nlohmann::json::object();
};

/**
 * @brief Applies every uncollected pickup within touch range of
 *        `playerPos` to `state`, in place.
 *
 * For each BSP entity in `entities` recognized as a health/armor/ammo/
 * weapon pickup, not yet in `state.collected`, and within
 * `kTouchRadius` of `playerPos`: applies its effect, marks it collected,
 * and schedules its respawn at `elapsed` + the pickup type's respawn
 * delay. Entities missing a parseable "origin"/"position" are skipped.
 * Logs one Info line per pickup collected, if `logger` is non-null.
 */
void ApplyPickupTouches(const nlohmann::json& entities,
                        const glm::vec3& playerPos, double elapsed,
                        PickupTouchState& state,
                        const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
