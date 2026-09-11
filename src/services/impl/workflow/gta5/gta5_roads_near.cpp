#include "services/interfaces/workflow/gta5/gta5_roads.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kLane = 5.f;  // metres across a lane

/// The spot on node a's link to b nearest `p`, if nearer than `best`
/// (squared, flat): in the lane running a to b.
bool Consider(const Gta5RoadNode& a, const Gta5RoadNode& b,
              const Gta5RoadLink& link, glm::vec2 p, float& best,
              Gta5RoadSpot& spot) {
    const glm::vec2 s(a.at.x, a.at.z), d = glm::vec2(b.at.x, b.at.z) - s;
    const float length2 = glm::dot(d, d);
    if (length2 < 1.f) return false;
    const float t = std::clamp(glm::dot(p - s, d) / length2, 0.f, 1.f);
    const glm::vec2 q = s + d * t;
    const float distance2 = glm::dot(p - q, p - q);
    if (distance2 >= best) return false;
    best = distance2;
    const glm::vec2 ahead = d / std::sqrt(length2);
    // Traffic keeps right: with lanes coming back, the node line is the
    // middle of the road and ours is half a lane to its right.
    const glm::vec2 right(-ahead.y, ahead.x);
    const glm::vec2 at = q + right * (link.back > 0 ? kLane * 0.5f : 0.f);
    spot.at = glm::vec3(at.x, a.at.y + (b.at.y - a.at.y) * t, at.y);
    spot.ahead = ahead;
    spot.yaw = std::atan2(ahead.x, ahead.y);
    return true;
}

}  // namespace

bool NearestGta5Road(const Gta5Roads& roads, const glm::vec3& near,
                     float reach, Gta5RoadSpot& spot) {
    const glm::vec2 p(near.x, near.z);
    const int span = static_cast<int>(std::ceil(reach / 100.f));
    const int cx = Gta5RoadCellOf(near.x), cz = Gta5RoadCellOf(near.z);
    float best = reach * reach;
    bool found = false;
    for (int dz = -span; dz <= span; ++dz) {
        for (int dx = -span; dx <= span; ++dx) {
            const auto cell = roads.cells.find(Gta5RoadCell(cx + dx, cz + dz));
            if (cell == roads.cells.end()) continue;
            for (const std::uint32_t i : cell->second) {
                const Gta5RoadNode& a = roads.nodes[i];
                for (std::uint32_t k = 0; k < a.linkCount; ++k) {
                    const Gta5RoadLink& link = roads.links[a.firstLink + k];
                    if (link.forward == 0 || link.to >= roads.nodes.size()) {
                        continue;
                    }
                    found |= Consider(a, roads.nodes[link.to], link, p, best,
                                      spot);
                }
            }
        }
    }
    return found;
}

}  // namespace sdl3cpp::services::impl
