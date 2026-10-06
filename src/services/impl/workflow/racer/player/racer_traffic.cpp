#include "services/interfaces/workflow/racer/player/racer_traffic.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kLookAhead = 40.f;   // metres in front that matter
constexpr float kLane = 7.f;         // half-width counted as "in line"
constexpr float kTooClose = 18.f;

}  // namespace

void AvoidRacerTraffic(const RacerPodState& pod,
                       const std::vector<const RacerPodState*>& others,
                       RacerPodInput& input) {
    const glm::vec3 forward = RacerPodForward(pod.heading);
    const glm::vec3 right(std::cos(pod.heading), 0.f, std::sin(pod.heading));
    for (const RacerPodState* other : others) {
        if (!other || other == &pod) continue;
        const glm::vec3 to = other->position - pod.position;
        const float ahead = glm::dot(to, forward);
        const float lateral = glm::dot(to, right);
        if (ahead <= 0.f || ahead > kLookAhead || std::fabs(lateral) > kLane) {
            continue;
        }
        // Pass on the side away from it, harder the nearer it is.
        const float urgency = 1.f - ahead / kLookAhead;
        input.steer += (lateral > 0.f ? -0.6f : 0.6f) * urgency;
        if (ahead < kTooClose && other->speed < pod.speed) {
            input.throttle = std::min(input.throttle, 0.4f);
            input.boost = false;
        }
    }
    input.steer = std::clamp(input.steer, -1.f, 1.f);
}

}  // namespace sdl3cpp::services::impl
