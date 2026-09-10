#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Player position: prefers q3.player_pos, falls back to camera.state's
/// "position" array, or (0,0,0) if neither is present.
glm::vec3 ResolveQ3PlayerPosition(WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
