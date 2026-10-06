#include "services/interfaces/workflow/racer/world/racer_ground.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Moller-Trumbore: does the segment p -> p + d (t in 0..1) hit the
/// triangle (a, b, c), from either side?
bool SegmentHits(const glm::vec3& p, const glm::vec3& d, const glm::vec3& a,
                 const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 e1 = b - a;
    const glm::vec3 e2 = c - a;
    const glm::vec3 h = glm::cross(d, e2);
    const float det = glm::dot(e1, h);
    if (std::fabs(det) < 1e-8f) return false;
    const float inv = 1.f / det;
    const glm::vec3 s = p - a;
    const float u = inv * glm::dot(s, h);
    if (u < 0.f || u > 1.f) return false;
    const glm::vec3 q = glm::cross(s, e1);
    const float v = inv * glm::dot(d, q);
    if (v < 0.f || u + v > 1.f) return false;
    const float t = inv * glm::dot(e2, q);
    return t >= 0.f && t <= 1.f;
}

}  // namespace

bool RacerWallBetween(const RacerGround& ground, const glm::vec3& from,
                      const glm::vec3& to) {
    return RacerWallHit(ground, from, to) >= 0;
}

int RacerWallHit(const RacerGround& ground, const glm::vec3& from,
                 const glm::vec3& to) {
    const glm::vec3 d = to - from;
    // A step is a few metres at most; check the cells at both ends.
    const long long keys[2] = {RacerGroundCell(ground, from.x, from.z),
                               RacerGroundCell(ground, to.x, to.z)};
    for (int k = 0; k < 2; ++k) {
        if (k == 1 && keys[1] == keys[0]) break;
        const auto it = ground.wallCells.find(keys[k]);
        if (it == ground.wallCells.end()) continue;
        for (int w : it->second) {
            if (SegmentHits(from, d, ground.walls[3 * w],
                            ground.walls[3 * w + 1], ground.walls[3 * w + 2])) {
                return w;
            }
        }
    }
    return -1;
}

}  // namespace sdl3cpp::services::impl
