#pragma once

#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"

#include <vector>

namespace sdl3cpp::q3 {

/**
 * @brief A* shortest path over a NavGraph.
 * @return Node indices from `startNode` to `goalNode` inclusive, or empty if
 *         either index is out of range or no path connects them.
 */
std::vector<int> FindNavPath(const NavGraph& graph, int startNode,
                             int goalNode);

}  // namespace sdl3cpp::q3
