#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Puts the player at `origin` and takes all motion out: the Bullet body
/// (transform and velocities) and the q3 movement state, which keeps its
/// own velocity and would otherwise build up a fall the whole time it is
/// held and release it in one go.
void PinBl4Player(WorkflowContext& context, const glm::vec3& origin);

}  // namespace sdl3cpp::services::impl
