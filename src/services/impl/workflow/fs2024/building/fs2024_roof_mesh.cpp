#include "services/interfaces/workflow/fs2024/building/fs2024_roof_mesh.hpp"

#include "services/interfaces/workflow/fs2024/building/fs2024_oriented_box.hpp"
#include "services/interfaces/workflow/fs2024/building/fs2024_roof_pitched.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

void AppendFlat(const std::vector<Point2>& footprint, float y,
                float metresPerTile, std::vector<BspRenderVertex>& vertices,
                std::vector<std::uint32_t>& indices) {
    const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
    for (const Point2& p : footprint) {
        vertices.push_back({p.x, y, p.y, p.x / metresPerTile,
                           p.y / metresPerTile, 0.f, 0.f, 0.f, 1.f, 0.f});
    }
    for (std::uint32_t index : TriangulatePolygon(footprint)) {
        indices.push_back(base + index);
    }
}

}  // namespace

float AppendRoofMesh(const std::vector<Point2>& footprint, float eaveY,
                     RoofShape shape, float rise, float metresPerTile,
                     std::vector<BspRenderVertex>& vertices,
                     std::vector<std::uint32_t>& indices) {
    if (footprint.size() < 3) return 0.f;
    if (shape == RoofShape::Flat || rise <= 0.f) {
        AppendFlat(footprint, eaveY, metresPerTile, vertices, indices);
        return 0.f;
    }
    const OrientedBox box = ComputeOrientedBox(footprint);
    if (box.halfWidth <= 0.f) {
        AppendFlat(footprint, eaveY, metresPerTile, vertices, indices);
        return 0.f;
    }
    // A hip's ridge stops short of the ends by the roof's own half
    // width, the classic 45-degree hip; a pyramid's ridge is a point.
    float ridgeHalf = box.halfLength;
    if (shape == RoofShape::Hipped) {
        ridgeHalf = std::max(0.f, box.halfLength - box.halfWidth);
    } else if (shape == RoofShape::Pyramidal) {
        ridgeHalf = 0.f;
    }
    AppendPitchedRoof(box, ridgeHalf, eaveY, rise, metresPerTile, vertices,
                     indices);
    return rise;
}

}  // namespace sdl3cpp::services::impl
