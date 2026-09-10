#pragma once

#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// AABB used to grid-sample the nav mesh: the default box, or one built
/// from `spawnPts` (bsp.spawn_points/bsp.entities), padded by 20 units
/// (5 vertically) around every spawn found.
struct NavBuildAabb {
    glm::vec3 min;
    glm::vec3 max;
};

NavBuildAabb ComputeNavBuildAabb(const nlohmann::json* spawnPts);

/**
 * @brief Grid-samples `aabb` in the XZ plane, casting a downward ray at
 * each point against `world`; a hit whose normal is walkable
 * (normal.y >= 0.7) becomes a nav node lifted 0.5 units above it.
 */
sdl3cpp::q3::NavGraphPtr SampleNavGraph(
    btDiscreteDynamicsWorld* world, const NavBuildAabb& aabb);

/**
 * @brief Connects every pair of nodes within 3.0 units that has a clear
 * line-of-sight ray between them.
 */
void ConnectNavNeighbors(
    btDiscreteDynamicsWorld* world, sdl3cpp::q3::NavGraph& graph);

}  // namespace sdl3cpp::services::impl
