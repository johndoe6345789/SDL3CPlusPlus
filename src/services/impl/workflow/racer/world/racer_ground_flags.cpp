#include "services/interfaces/workflow/racer/world/racer_ground.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

std::uint32_t RacerGroundFlags(const RacerGround& ground, float x, float z,
                               float ceiling) {
    const auto it = ground.cells.find(RacerGroundCell(ground, x, z));
    if (it == ground.cells.end()) return 0;
    std::uint32_t found = 0;
    float best = -1e30f;
    for (int t : it->second) {
        const glm::vec3& a = ground.triangles[3 * t];
        const glm::vec3& b = ground.triangles[3 * t + 1];
        const glm::vec3& c = ground.triangles[3 * t + 2];
        const float d =
            (b.z - c.z) * (a.x - c.x) + (c.x - b.x) * (a.z - c.z);
        if (std::fabs(d) < 1e-6f) continue;
        const float u = ((b.z - c.z) * (x - c.x) + (c.x - b.x) * (z - c.z)) / d;
        const float v = ((c.z - a.z) * (x - c.x) + (a.x - c.x) * (z - c.z)) / d;
        const float w = 1.f - u - v;
        if (u < -1e-4f || v < -1e-4f || w < -1e-4f) continue;
        const float h = u * a.y + v * b.y + w * c.y;
        if (h <= ceiling && h > best) {
            best = h;
            found = static_cast<std::size_t>(t) < ground.flags.size()
                        ? ground.flags[t]
                        : 0;
        }
    }
    return found;
}

}  // namespace sdl3cpp::services::impl
