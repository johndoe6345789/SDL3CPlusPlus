#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// The smallest-area rectangle enclosing a footprint, in engine
/// (x, z) metres: its centre, its half extents along its own axes,
/// and the direction of its longer axis. A real building's ridge runs
/// along this rectangle's long axis, which is what makes a gabled or
/// hipped roof sit the right way round on a terrace or a semi.
struct OrientedBox {
    Point2 centre;
    Point2 longAxis{1.f, 0.f};  ///< unit vector, the ridge direction
    float halfLength = 0.f;     ///< along longAxis
    float halfWidth = 0.f;      ///< across it
};

/// Rotating calipers over the footprint's convex hull: the minimum
/// area rectangle always shares an edge with the hull.
OrientedBox ComputeOrientedBox(const std::vector<Point2>& footprint);

}  // namespace sdl3cpp::services::impl
