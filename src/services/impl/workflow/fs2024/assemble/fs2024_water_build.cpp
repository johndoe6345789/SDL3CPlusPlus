#include "services/interfaces/workflow/fs2024/assemble/fs2024_water_build.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kBedDepth = 3.f;  ///< ground under water, below its level

/// The lowest ground inside `ring`: its grid points, and the ground under
/// its own outline.
float LowestGround(const std::vector<Point2>& ring,
                   const Fs2024Heightfield& field) {
    float lowest = 1e9f;
    for (const Point2& p : ring) {
        lowest = std::min(lowest, Fs2024HeightAt(field, p.x, p.y));
    }
    for (int r = 0; r < field.rows; ++r) {
        for (int c = 0; c < field.columns; ++c) {
            const glm::vec3 at = field.Position(c, r);
            if (Fs2024RingContains(ring, {at.x, at.z})) {
                lowest = std::min(lowest, at.y);
            }
        }
    }
    return lowest;
}

void Carve(const std::vector<Point2>& ring, float level,
           Fs2024Heightfield& field) {
    for (int r = 0; r < field.rows; ++r) {
        for (int c = 0; c < field.columns; ++c) {
            const glm::vec3 at = field.Position(c, r);
            if (!Fs2024RingContains(ring, {at.x, at.z})) continue;
            float& h = field.heights[static_cast<std::size_t>(r) *
                                         field.columns + c];
            h = std::min(h, level - kBedDepth);
        }
    }
}

}  // namespace

Fs2024WaterBuild BuildFs2024Water(const std::vector<Fs2024VecShape>& water,
                                  Fs2024Heightfield& field) {
    Fs2024WaterBuild build;
    build.mesh.min = glm::vec3(1e30f);
    build.mesh.max = glm::vec3(-1e30f);
    for (const Fs2024VecShape& shape : water) {
        const float level = LowestGround(shape.points, field);
        build.levels.push_back(level);
        Carve(shape.points, level, field);
        const auto first =
            static_cast<std::uint32_t>(build.mesh.vertices.size());
        for (const Point2& p : shape.points) {
            build.mesh.vertices.push_back(BspRenderVertex{
                p.x, level, p.y, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f});
            const glm::vec3 at(p.x, level, p.y);
            build.mesh.min = glm::min(build.mesh.min, at);
            build.mesh.max = glm::max(build.mesh.max, at);
        }
        for (const std::uint32_t i : TriangulatePolygon(shape.points)) {
            build.mesh.indices.push_back(first + i);
        }
    }
    const auto [lo, hi] =
        std::minmax_element(field.heights.begin(), field.heights.end());
    if (lo != field.heights.end()) {
        field.minHeight = *lo;
        field.maxHeight = *hi;
    }
    return build;
}

}  // namespace sdl3cpp::services::impl
