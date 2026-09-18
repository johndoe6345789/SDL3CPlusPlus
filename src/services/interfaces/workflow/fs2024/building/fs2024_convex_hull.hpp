#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// The points' convex hull, counter-clockwise, by Andrew's monotone
/// chain. Duplicate points are dropped; fewer than three distinct
/// points come back unchanged.
std::vector<Point2> ConvexHull(std::vector<Point2> points);

}  // namespace sdl3cpp::services::impl
