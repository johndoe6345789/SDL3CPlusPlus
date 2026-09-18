#include "services/interfaces/workflow/fs2024/assemble/fs2024_road_build.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kStep = 12.f;     ///< drape samples, metres apart
constexpr float kLift = 0.3f;     ///< above the ground, before depth bias
constexpr float kTexture = 8.f;   ///< metres of road per texture repeat

void AddRibbon(const Fs2024VecShape& road, float width,
               const std::vector<Fs2024VecShape>& water,
               const std::vector<float>& levels,
               const Fs2024Heightfield& field, Fs2024TerrainChunkMesh& mesh) {
    std::vector<Point2> at;  // the polyline, subdivided to follow the ground
    for (std::size_t i = 0; i + 1 < road.points.size(); ++i) {
        const Point2 a = road.points[i], b = road.points[i + 1];
        const int n = std::max(1, static_cast<int>(
            std::hypot(b.x - a.x, b.y - a.y) / kStep));
        for (int k = 0; k < n; ++k) {
            const float t = static_cast<float>(k) / static_cast<float>(n);
            at.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t});
        }
    }
    if (road.points.size() >= 2) at.push_back(road.points.back());
    float along = 0.f;
    for (std::size_t i = 0; i < at.size(); ++i) {
        const Point2 prev = at[i > 0 ? i - 1 : i];
        const Point2 next = at[i + 1 < at.size() ? i + 1 : i];
        const float dx = next.x - prev.x, dy = next.y - prev.y;
        const float length = std::max(std::hypot(dx, dy), 1e-4f);
        const float nx = -dy / length * width * 0.5f;
        const float ny = dx / length * width * 0.5f;
        if (i > 0) along += std::hypot(at[i].x - at[i - 1].x,
                                       at[i].y - at[i - 1].y);
        float h = Fs2024HeightAt(field, at[i].x, at[i].y);
        if (road.Bridge()) {
            h = Fs2024BridgeDeck(at[i], h, road.level, water, levels);
        }
        h += kLift;
        const float v = along / kTexture;
        const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
        for (const float side : {1.f, -1.f}) {
            const glm::vec3 p(at[i].x + nx * side, h, at[i].y + ny * side);
            mesh.vertices.push_back(BspRenderVertex{
                p.x, p.y, p.z, side > 0.f ? 0.f : width / kTexture, v,
                0.f, 0.f, 0.f, 1.f, 0.f});
            mesh.min = glm::min(mesh.min, p);
            mesh.max = glm::max(mesh.max, p);
        }
        if (i > 0) {
            mesh.indices.insert(mesh.indices.end(),
                                {base - 2, base, base - 1, base - 1, base,
                                 base + 1});
        }
    }
}

}  // namespace

Fs2024TerrainChunkMesh BuildFs2024Roads(
    const std::vector<Fs2024VecShape>& roads,
    const std::vector<Fs2024VecShape>& water,
    const std::vector<float>& levels, const Fs2024Heightfield& field) {
    Fs2024TerrainChunkMesh mesh;
    mesh.min = glm::vec3(1e30f);
    mesh.max = glm::vec3(-1e30f);
    for (const Fs2024VecShape& road : roads) {
        const float width = Fs2024RoadWidth(road.classBit);
        if (width <= 0.f || road.Tunnel()) continue;
        AddRibbon(road, width, water, levels, field, mesh);
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
