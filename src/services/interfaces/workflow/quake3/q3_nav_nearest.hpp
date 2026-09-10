#pragma once

#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"

namespace sdl3cpp::q3 {

/// The graph node closest (by straight-line distance) to a world position.
int NearestNavNode(const NavGraph& graph, const glm::vec3& pos);

}  // namespace sdl3cpp::q3
