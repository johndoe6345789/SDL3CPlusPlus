#pragma once

#include "services/interfaces/workflow/quake3/q3_nav_aabb.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Grid-samples `aabb` in the XZ plane, casting a downward ray at
 * each point against `world`; a hit whose normal is walkable
 * (normal.y >= 0.7) becomes a nav node lifted 0.5 units above it.
 */
sdl3cpp::q3::NavGraphPtr SampleNavGraph(btDiscreteDynamicsWorld* world,
                                        const NavBuildAabb& aabb);

}  // namespace sdl3cpp::services::impl
