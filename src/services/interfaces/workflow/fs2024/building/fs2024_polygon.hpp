#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A 2D point, kept dependency-free of glm here since this is pure
/// geometry shared between the roof triangulator and its tests.
struct Point2 {
    float x = 0.f, y = 0.f;
};

/// Ear-clipping triangulation of a simple polygon (no holes, may be
/// convex or not, must not self-intersect): indices into `polygon`,
/// three per triangle, in the same winding order the polygon was
/// given in. Degenerates to nothing for fewer than 3 points.
std::vector<std::uint32_t> TriangulatePolygon(
    const std::vector<Point2>& polygon);

}  // namespace sdl3cpp::services::impl
