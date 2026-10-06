#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

void PlaceRacerPod(const RacerWorldState& state, RacerPodState& pod,
                   int index, float speed, float lateral) {
    const int count = static_cast<int>(state.lapPoints.size());
    if (count < 2) return;
    index = ((index % count) + count) % count;
    const glm::vec3 to =
        state.lapPoints[(index + 1) % count] - state.lapPoints[index];
    pod = RacerPodState{};
    pod.heading = std::atan2(to.x, -to.z);
    pod.speed = speed;
    const glm::vec3 right(std::cos(pod.heading), 0.f, std::sin(pod.heading));
    const glm::vec3 at = state.lapPoints[index] + right * lateral;
    const auto floor = RacerGroundHeight(state.ground, at.x, at.z,
                                         at.y + state.podSpec.stepHeight);
    pod.position = at;
    pod.position.y = (floor ? *floor : at.y) + state.podSpec.hoverHeight;
}

void PlaceRacerPodOnLap(RacerWorldState& state, int index, float speed) {
    PlaceRacerPod(state, state.pod, index, speed, 0.f);
}

}  // namespace sdl3cpp::services::impl
