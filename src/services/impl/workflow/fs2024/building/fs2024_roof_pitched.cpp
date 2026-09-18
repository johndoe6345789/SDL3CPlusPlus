#include "services/interfaces/workflow/fs2024/building/fs2024_roof_pitched.hpp"

#include <array>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

struct Vertex3 {
    float x = 0.f, y = 0.f, z = 0.f;
};

Vertex3 Corner(const OrientedBox& box, float along, float across, float y) {
    return {box.centre.x + box.longAxis.x * along - box.longAxis.y * across,
            y,
            box.centre.y + box.longAxis.y * along + box.longAxis.x * across};
}

/// Appends a triangle, flipped if need be so its front faces
/// outwards: up for a slope, and away from `centre` for a vertical
/// gable end, whose normal is horizontal and so has no "up" to sort
/// it by.
void AppendFace(const std::array<Vertex3, 3>& face, const Point2& centre,
                float metresPerTile,
                std::vector<BspRenderVertex>& vertices,
                std::vector<std::uint32_t>& indices) {
    const float ux = face[1].x - face[0].x, uy = face[1].y - face[0].y;
    const float uz = face[1].z - face[0].z;
    const float vx = face[2].x - face[0].x, vy = face[2].y - face[0].y;
    const float vz = face[2].z - face[0].z;
    float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz;
    float nz = ux * vy - uy * vx;
    const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (length < 1e-9f) return;
    bool flip = ny < 0.f;
    if (std::abs(ny) < 1e-4f * length) {
        const float mx = (face[0].x + face[1].x + face[2].x) / 3.f - centre.x;
        const float mz = (face[0].z + face[1].z + face[2].z) / 3.f - centre.y;
        flip = nx * mx + nz * mz < 0.f;
    }
    const float scale = (flip ? -1.f : 1.f) / length;
    nx *= scale; ny *= scale; nz *= scale;

    const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
    for (const Vertex3& point : face) {
        vertices.push_back({point.x, point.y, point.z,
                           point.x / metresPerTile,
                           point.z / metresPerTile, 0.f, 0.f, nx, ny, nz});
    }
    if (flip) {
        indices.insert(indices.end(), {base, base + 2, base + 1});
    } else {
        indices.insert(indices.end(), {base, base + 1, base + 2});
    }
}

}  // namespace

void AppendPitchedRoof(const OrientedBox& box, float ridgeHalf, float eaveY,
                       float rise, float metresPerTile,
                       std::vector<BspRenderVertex>& vertices,
                       std::vector<std::uint32_t>& indices) {
    const float halfLength = box.halfLength, halfWidth = box.halfWidth;
    const float ridgeY = eaveY + rise;
    const Vertex3 e0 = Corner(box, -halfLength, -halfWidth, eaveY);
    const Vertex3 e1 = Corner(box, halfLength, -halfWidth, eaveY);
    const Vertex3 e2 = Corner(box, halfLength, halfWidth, eaveY);
    const Vertex3 e3 = Corner(box, -halfLength, halfWidth, eaveY);
    const Vertex3 r0 = Corner(box, -ridgeHalf, 0.f, ridgeY);
    const Vertex3 r1 = Corner(box, ridgeHalf, 0.f, ridgeY);

    const Point2& centre = box.centre;
    AppendFace({e0, e1, r1}, centre, metresPerTile, vertices, indices);
    AppendFace({e0, r1, r0}, centre, metresPerTile, vertices, indices);
    AppendFace({e2, e3, r0}, centre, metresPerTile, vertices, indices);
    AppendFace({e2, r0, r1}, centre, metresPerTile, vertices, indices);
    AppendFace({e0, r0, e3}, centre, metresPerTile, vertices, indices);
    AppendFace({e1, e2, r1}, centre, metresPerTile, vertices, indices);
}

}  // namespace sdl3cpp::services::impl
