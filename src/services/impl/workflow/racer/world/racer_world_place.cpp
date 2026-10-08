#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <cmath>
#include <optional>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kFloorBelowLine = 6.f;  // metres; deeper is a gap
constexpr int kFloorSearch = 48;        // lap points (about 400 m)

/// The floor for a point on the line: just above it first (higher is
/// usually another level, like Malastare's crossover), then up to 10 m
/// above (where the line runs buried, as at Grabvine's start). Empty
/// over a gap.
std::optional<float> FloorNearLine(const RacerGround& ground,
                                   const glm::vec3& p) {
    for (const float above : {2.f, 10.f}) {
        const auto floor = RacerGroundHeight(ground, p.x, p.z, p.y + above);
        if (floor && *floor > p.y - kFloorBelowLine) return floor;
    }
    return std::nullopt;
}

}  // namespace

void PlaceRacerPod(const RacerWorldState& state, RacerPodState& pod,
                   int index, float speed, float lateral) {
    const int count = static_cast<int>(state.lapPoints.size());
    if (count < 2) return;
    index = ((index % count) + count) % count;
    // Never over a jump's gap: on to the first point with floor close
    // to the line (which can run a little under the ground), within a
    // short way; failing that, the point asked for.
    const int asked = index;
    for (int tries = 0; tries <= kFloorSearch; ++tries) {
        if (FloorNearLine(state.ground, state.lapPoints[index])) break;
        index = tries == kFloorSearch ? asked : (index + 1) % count;
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
    // A grid slot beside the line may hang over a drop: move it in
    // toward the line until there is floor under it.
    glm::vec3 at = state.lapPoints[index];
    std::optional<float> floor;
    for (const float share : {1.f, 0.5f, 0.f}) {
        at = state.lapPoints[index] + right * (lateral * share);
        floor = FloorNearLine(state.ground, at);
        if (floor) break;
    }
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
