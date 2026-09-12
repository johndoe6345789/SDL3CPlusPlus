#include "services/interfaces/workflow/gta5/traffic/gta5_traffic.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

/// A bearing folded onto half a turn: a road and the same road the
/// other way are one arm of a crossing, not two.
float Folded(float radians) {
    float out = std::fmod(radians, kPi);
    return out < 0.f ? out + kPi : out;
}

/// How far apart two folded bearings are, at most a quarter turn.
float Apart(float a, float b) {
    const float gap = std::fabs(Folded(a) - Folded(b));
    return std::min(gap, kPi - gap);
}

}  // namespace

void StepGta5Lights(Gta5Traffic& traffic, const Gta5Roads& roads,
                    const glm::vec3& at, float dt) {
    const float keep = traffic.far + 60.f;
    const auto gone = std::remove_if(
        traffic.junctions.begin(), traffic.junctions.end(),
        [&](const Gta5Junction& j) {
            return j.node >= roads.nodes.size() ||
                   glm::distance(roads.nodes[j.node].at, at) > keep;
        });
    traffic.junctions.erase(gone, traffic.junctions.end());
    for (Gta5Junction& j : traffic.junctions) {
        j.clock = std::fmod(j.clock + dt, traffic.cycle);
    }
    Gta5FindJunctions(traffic, roads, at);
}

bool Gta5LightOpen(const Gta5Traffic& traffic, std::uint32_t node,
                   float heading) {
    for (const Gta5Junction& j : traffic.junctions) {
        if (j.node != node) continue;
        // Half the cycle each way, less an amber at the end of it that
        // stops the traffic already committed from being caught out.
        const float half = traffic.cycle * 0.5f;
        const bool firstHalf = j.clock < half;
        const float into = firstHalf ? j.clock : j.clock - half;
        if (into > half - traffic.amber) return false;
        const bool along = Apart(heading, j.axis) < kPi * 0.25f;
        return along == firstHalf;
    }
    return true;  // not a crossing: nothing to wait for
}

}  // namespace sdl3cpp::services::impl
