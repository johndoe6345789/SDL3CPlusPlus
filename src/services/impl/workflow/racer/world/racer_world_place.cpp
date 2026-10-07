#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kFloorBelowLine = 6.f;  // metres; deeper is a gap

}  // namespace

void PlaceRacerPod(const RacerWorldState& state, RacerPodState& pod,
                   int index, float speed, float lateral) {
    const int count = static_cast<int>(state.lapPoints.size());
    if (count < 2) return;
    index = ((index % count) + count) % count;
    // Never over a jump's gap: on to the first point with floor close
    // under the line.
    for (int tries = 0; tries < count; ++tries) {
        const glm::vec3& p = state.lapPoints[index];
        const auto floor = RacerGroundHeight(state.ground, p.x, p.z,
                                             p.y + state.podSpec.stepHeight);
        if (floor && *floor > p.y - kFloorBelowLine) break;
        index = (index + 1) % count;
    }
    const glm::vec3 to =
        state.lapPoints[(index + 1) % count] - state.lapPoints[index];
    // A fresh state, keeping the pod's own body (size and mass).
    RacerPodState fresh;
    fresh.bodyFront = pod.bodyFront;
    fresh.bodyBack = pod.bodyBack;
    fresh.bodyHalfWidth = pod.bodyHalfWidth;
    fresh.mass = pod.mass;
    pod = fresh;
    pod.heading = std::atan2(to.x, -to.z);
    pod.speed = speed;
    const glm::vec3 right(std::cos(pod.heading), 0.f, std::sin(pod.heading));
    const glm::vec3 at = state.lapPoints[index] + right * lateral;
    const auto floor = RacerGroundHeight(state.ground, at.x, at.z,
                                         at.y + state.podSpec.stepHeight);
    pod.position = at;
    pod.position.y = (floor ? *floor : at.y) + state.podSpec.hoverHeight;
}

void SetRacerPodBody(RacerPodState& pod, const RacerPodRig& rig,
                     const RacerPodSpec& spec) {
    pod.bodyFront = rig.front;
    pod.bodyBack = rig.back;
    pod.bodyHalfWidth = rig.halfWidth;
    pod.mass = spec.bumpMass;
}

void PlaceRacerPodOnLap(RacerWorldState& state, int index, float speed) {
    PlaceRacerPod(state, state.pod, index, speed, 0.f);
}

}  // namespace sdl3cpp::services::impl
