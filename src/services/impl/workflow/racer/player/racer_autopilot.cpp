#include "services/interfaces/workflow/racer/player/racer_autopilot.hpp"

#include "services/interfaces/workflow/racer/render/racer_camera_math.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

// Aim this far ahead along the lap, a little further at speed.
constexpr float kLookAheadMetres = 18.f;
constexpr float kLookAheadSeconds = 0.25f;
constexpr float kBrakeLookAhead = 45.f;

/// Signed turn from the pod's heading to the lap point `distance` metres
/// ahead of `from` along the lap.
float TurnTo(const RacerPodState& pod, const std::vector<glm::vec3>& lap,
             int from, float distance) {
    const int count = static_cast<int>(lap.size());
    int index = from;
    float run = 0.f;
    for (int step = 0; step < count && run < distance; ++step) {
        const int next = (index + 1) % count;
        run += glm::length(lap[next] - lap[index]);
        index = next;
    }
    const glm::vec3 to = lap[index] - pod.position;
    return RacerAngleDelta(pod.heading, std::atan2(to.x, -to.z));
}

}  // namespace

RacerPodInput RacerAutopilot(const RacerPodState& pod,
                             const std::vector<glm::vec3>& lapPoints,
                             int nearestPoint) {
    RacerPodInput input;
    const int count = static_cast<int>(lapPoints.size());
    if (count < 2 || nearestPoint < 0) return input;
    const float aim =
        kLookAheadMetres + kLookAheadSeconds * std::max(0.f, pod.speed);
    const float turn = TurnTo(pod, lapPoints, nearestPoint, aim);
    // How sharply the lap bends further on decides the speed to carry.
    const float bend = std::fabs(
        TurnTo(pod, lapPoints, nearestPoint, aim + kBrakeLookAhead));
    input.steer = std::clamp(turn * 2.5f, -1.f, 1.f);
    const float wantedSpeed =
        145.f * std::clamp(1.25f - 1.4f * bend, 0.25f, 1.f);
    input.throttle = pod.speed > wantedSpeed + 8.f ? -0.6f : 1.f;
    input.boost = bend < 0.15f && pod.heat < 0.6f;
    input.repair = RacerPodDamage(pod) > 0.5f && pod.heat < 0.2f;
    return input;
}

}  // namespace sdl3cpp::services::impl
