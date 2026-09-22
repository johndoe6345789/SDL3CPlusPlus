#pragma once

#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// True when `at` lies on a cell whose tile carries road.
bool StuntsOnRoad(const StuntsWorldState& state, const glm::vec3& at);

/// Keeps `at` inside the 30x30 grid the game itself never leaves.
glm::vec3 StuntsClampToGrid(const StuntsWorldState& state,
                            const glm::vec3& at);

}  // namespace sdl3cpp::services::impl
