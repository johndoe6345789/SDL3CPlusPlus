#pragma once

#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// Prefers the explicit `q3.player_pos` override; falls back to
/// `camera.state.position`, or the origin if neither is set.
glm::vec3 ReadBotUpdatePlayerPosition(const WorkflowContext& context);

/// Null unless q3.nav.build has already populated the shared nav graph.
const q3::NavGraph* ReadBotUpdateNavGraph(const WorkflowContext& context);

/// True if nothing blocks the line from `from` to `to` in the physics world;
/// with no world, falls back to "close enough" (`distance < chaseRange`).
bool HasLineOfSightToPlayer(btDiscreteDynamicsWorld* world,
                            const glm::vec3& from, const glm::vec3& to,
                            float distance, float chaseRange);

/// idle / chase / shoot, from distance-to-player and line-of-sight.
std::string DetermineBotState(float distance, bool canSeePlayer,
                              float shootRange, float chaseRange);

}  // namespace sdl3cpp::services::impl
