#include "services/interfaces/workflow/fs2024/fs2024_building_mesh.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

BspRenderVertex Vertex(float x, float y, float z, float nx, float ny,
                      float nz) {
    return {x, y, z, 0.f, 0.f, 0.f, 0.f, nx, ny, nz};
}

/// Reverses point order if needed so the shoelace sum is negative --
/// this engine's own convention for an upward-facing horizontal
/// triangle (see BuildFs2024TerrainChunk's nw/sw/ne winding).
std::vector<Point2> NormalisedWinding(std::vector<Point2> footprint) {
    float sum = 0.f;
    for (std::size_t i = 0; i < footprint.size(); ++i) {
        const Point2& a = footprint[i];
        const Point2& b = footprint[(i + 1) % footprint.size()];
        sum += a.x * b.y - b.x * a.y;
    }
    if (sum > 0.f) {
        std::reverse(footprint.begin(), footprint.end());
    }
    return footprint;
}

void AppendWall(const Point2& a, const Point2& b, float height,
               std::vector<BspRenderVertex>& vertices,
               std::vector<std::uint32_t>& indices) {
    const float dx = b.x - a.x, dz = b.y - a.y;
    const float length = std::sqrt(dx * dx + dz * dz);
    if (length < 1e-4f) return;  // a duplicated node; no wall to build
    const float nx = -dz / length, nz = dx / length;

    const auto base = static_cast<std::uint32_t>(vertices.size());
    vertices.push_back(Vertex(a.x, 0.f, a.y, nx, 0.f, nz));
    vertices.push_back(Vertex(b.x, 0.f, b.y, nx, 0.f, nz));
    vertices.push_back(Vertex(b.x, height, b.y, nx, 0.f, nz));
    vertices.push_back(Vertex(a.x, height, a.y, nx, 0.f, nz));
    for (std::uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u}) {
        indices.push_back(base + i);
    }
}

void AppendRoof(const std::vector<Point2>& footprint, float height,
               std::vector<BspRenderVertex>& vertices,
               std::vector<std::uint32_t>& indices) {
    const auto triangles = TriangulatePolygon(footprint);
    const auto base = static_cast<std::uint32_t>(vertices.size());
    for (const Point2& p : footprint) {
        vertices.push_back(Vertex(p.x, height, p.y, 0.f, 1.f, 0.f));
    }
    for (std::uint32_t index : triangles) indices.push_back(base + index);
}

}  // namespace

void AppendBuildingMesh(const std::vector<Point2>& rawFootprint,
                       float height,
                       std::vector<BspRenderVertex>& vertices,
                       std::vector<std::uint32_t>& indices) {
    if (rawFootprint.size() < 3 || height <= 0.f) return;
    const std::vector<Point2> footprint = NormalisedWinding(rawFootprint);

    for (std::size_t i = 0; i < footprint.size(); ++i) {
        AppendWall(footprint[i], footprint[(i + 1) % footprint.size()],
                  height, vertices, indices);
    }
    AppendRoof(footprint, height, vertices, indices);
}

}  // namespace sdl3cpp::services::impl
