#include "services/interfaces/workflow/racer/world/racer_ground.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

long long CellKey(int cx, int cz) {
    return (static_cast<long long>(cx) << 32) ^
           static_cast<unsigned int>(cz);
}

int CellOf(float v, float size) {
    return static_cast<int>(std::floor(v / size));
}

/// Height of the triangle's plane at (x, z) when (x, z) is inside it.
std::optional<float> HeightOn(const glm::vec3& a, const glm::vec3& b,
                              const glm::vec3& c, float x, float z) {
    const float d = (b.z - c.z) * (a.x - c.x) + (c.x - b.x) * (a.z - c.z);
    if (std::fabs(d) < 1e-6f) return std::nullopt;
    const float u = ((b.z - c.z) * (x - c.x) + (c.x - b.x) * (z - c.z)) / d;
    const float v = ((c.z - a.z) * (x - c.x) + (a.x - c.x) * (z - c.z)) / d;
    const float w = 1.f - u - v;
    if (u < -1e-4f || v < -1e-4f || w < -1e-4f) return std::nullopt;
    return u * a.y + v * b.y + w * c.y;
}

}  // namespace

void AddRacerGroundTriangle(RacerGround& ground, const glm::vec3& a,
                            const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 n = glm::cross(b - a, c - a);
    const float length = glm::length(n);
    // Only surfaces a pod can ride on: walls are handled by step height.
    if (length < 1e-6f || std::fabs(n.y) / length < 0.5f) return;
    const int index = static_cast<int>(ground.triangles.size() / 3);
    ground.triangles.insert(ground.triangles.end(), {a, b, c});
    const float s = ground.cellSize;
    const int x0 = CellOf(std::min({a.x, b.x, c.x}), s);
    const int x1 = CellOf(std::max({a.x, b.x, c.x}), s);
    const int z0 = CellOf(std::min({a.z, b.z, c.z}), s);
    const int z1 = CellOf(std::max({a.z, b.z, c.z}), s);
    for (int cx = x0; cx <= x1; ++cx) {
        for (int cz = z0; cz <= z1; ++cz) {
            ground.cells[CellKey(cx, cz)].push_back(index);
        }
    }
}

std::optional<float> RacerGroundHeight(const RacerGround& ground, float x,
                                       float z, float ceiling) {
    const auto it = ground.cells.find(CellKey(CellOf(x, ground.cellSize),
                                              CellOf(z, ground.cellSize)));
    if (it == ground.cells.end()) return std::nullopt;
    std::optional<float> best;
    for (int t : it->second) {
        const auto h = HeightOn(ground.triangles[3 * t],
                                ground.triangles[3 * t + 1],
                                ground.triangles[3 * t + 2], x, z);
        if (h && *h <= ceiling && (!best || *h > *best)) best = h;
    }
    return best;
}

}  // namespace sdl3cpp::services::impl
