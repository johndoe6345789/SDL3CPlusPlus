#pragma once

#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Connects every pair of nodes within 3.0 units that has a clear
 * line-of-sight ray between them.
 */
void ConnectNavNeighbors(btDiscreteDynamicsWorld* world,
                         sdl3cpp::q3::NavGraph& graph);

}  // namespace sdl3cpp::services::impl
