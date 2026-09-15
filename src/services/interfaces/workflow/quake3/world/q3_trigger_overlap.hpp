#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

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
