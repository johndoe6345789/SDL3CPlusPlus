#include "services/interfaces/workflow/racer/player/racer_autopilot.hpp"

#include "services/interfaces/workflow/racer/render/racer_camera_math.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

// Follow the line's direction this far ahead, a little further at
// speed, and steer back onto it in proportion to the sideways error.
constexpr float kLookAheadMetres = 6.f;
constexpr float kLookAheadSeconds = 0.12f;
constexpr float kCrossTrackGain = 4.f;
constexpr float kPlanHorizon = 450.f;   // metres of lap the plan covers

/// The lap point `distance` metres on from `from`.
int PointAhead(const std::vector<glm::vec3>& lap, int from, float distance) {
    const int count = static_cast<int>(lap.size());
    int index = from;
    float run = 0.f;
    for (int step = 0; step < count && run < distance; ++step) {
        const int next = (index + 1) % count;
        run += glm::length(lap[next] - lap[index]);
        index = next;
    }
    return index;
}

/// The line's direction at `i`, over a few metres: neighbouring
/// samples can coincide, and their direction would be meaningless.
float LineHeading(const std::vector<glm::vec3>& lap, int i) {
    const glm::vec3 d = lap[PointAhead(lap, i, 4.f)] - lap[i];
    return std::atan2(d.x, -d.z);
}

}  // namespace

RacerPodInput RacerAutopilot(const RacerPodState& pod,
                             const std::vector<glm::vec3>& lapPoints,
                             int nearestPoint, const RacerPodSpec& spec) {
    RacerPodInput input;
    const int count = static_cast<int>(lapPoints.size());
    if (count < 2 || nearestPoint < 0) return input;
    const float aim =
        kLookAheadMetres + kLookAheadSeconds * std::max(0.f, pod.speed);
    // Stanley steering: the line's heading ahead, corrected by how far
    // to its side the pod is (right of the line is positive).
    const float here = LineHeading(lapPoints, nearestPoint);
    const glm::vec3 right(std::cos(here), 0.f, std::sin(here));
    const float lateral =
        glm::dot(pod.position - lapPoints[nearestPoint], right);
    const float wanted =
        LineHeading(lapPoints, PointAhead(lapPoints, nearestPoint, aim)) -
        std::atan(kCrossTrackGain * lateral /
                  (std::max(0.f, pod.speed) + 10.f));
    const float turn = RacerAngleDelta(pod.heading, wanted);
    input.steer = std::clamp(turn * 6.f, -1.f, 1.f);
    const float planned =
        RacerPlannedSpeed(lapPoints, nearestPoint, spec, kPlanHorizon);
    const float over = pod.speed - planned;
    input.throttle = over > 12.f ? -1.f : (over > 2.f ? 0.f : 1.f);
    input.boost = planned > 0.97f * spec.boostSpeed && pod.heat < 0.6f &&
                  std::fabs(turn) < 0.1f;
    input.repair = RacerPodDamage(pod) > 0.5f && pod.heat < 0.2f;
    return input;
}

}  // namespace sdl3cpp::services::impl
