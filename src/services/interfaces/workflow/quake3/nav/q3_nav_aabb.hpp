#pragma once

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

}  // namespace sdl3cpp::services::impl
