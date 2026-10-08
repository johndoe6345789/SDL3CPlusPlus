#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// A race gantry at each checkpoint, in world space: two posts and a banner
/// across the road, turned to face the next checkpoint. Built once, so the
/// draw needs no per-frame transform.
GeometryPlaneMesh BuildSwitchbackMarqueeMesh(
    const std::vector<glm::vec3>& points);

}  // namespace sdl3cpp::services::impl
