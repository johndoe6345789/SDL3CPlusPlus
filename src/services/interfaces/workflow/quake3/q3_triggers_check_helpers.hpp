#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <memory>

namespace sdl3cpp::services::impl {

/// Reads a BSP entity's numeric field, which may be stored as either a
/// JSON number or (as BSP entity dictionaries always are) a string.
float EntFloat(const nlohmann::json& ent, const char* key, float def);

/// Parses a Q3 entity "x y z" origin string into world units, applying the
/// Quake-units-to-meters scale (default 1/32).
glm::vec3 ParseOrigin(const std::string& s, float scale = 0.03125f);

/**
 * @brief One-time parse of `bsp.entities` into q3.triggers.check's trigger
 * list and destination index, if not already done this map load.
 *
 * Builds `q3.trigger_list` (trigger_push/trigger_teleport/trigger_hurt
 * entities, each with classname/origin/target/dmg) and
 * `q3.trigger_dest_index` (targetname -> origin, from target_position and
 * misc_teleporter_dest entities), then sets `q3.triggers_loaded` so this
 * only runs once. A no-op if `q3.triggers_loaded` is already true.
 */
void LoadQ3TriggersIfNeeded(WorkflowContext& context,
                            const std::shared_ptr<ILogger>& logger);

/// The player position q3.triggers.check overlap-tests against: the
/// explicit `q3.player_pos` override, else `camera.state.position`, else
/// the origin.
glm::vec3 ReadQ3TriggerPlayerPos(const WorkflowContext& context);

/**
 * @brief Applies any trigger whose origin is within 1.5 units of
 * `playerPos`: trigger_push sets a launch-velocity override toward its
 * target's destination, trigger_teleport sets a teleport destination, and
 * trigger_hurt accumulates `q3.pending_damage`.
 */
void ApplyQ3TriggerOverlaps(WorkflowContext& context,
                           const nlohmann::json& triggerList,
                           const nlohmann::json* destIndex,
                           const glm::vec3& playerPos);

}  // namespace sdl3cpp::services::impl
