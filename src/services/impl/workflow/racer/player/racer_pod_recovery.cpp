#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kFallSeconds = 5.f;    // airborne this long: off course
constexpr float kVoidSeconds = 1.f;    // over nothing this long: off
constexpr float kStuckSeconds = 2.f;   // pinned this long: put back
constexpr float kStallSeconds = 6.f;   // no lap progress this long

}  // namespace

RacerRecoveryReason UpdateRacerRecovery(RacerRecovery& recovery,
                                        const RacerWorldState& state,
                                        float throttle, float dt) {
    const RacerPodState& pod = state.pod;
    const bool overVoid = !RacerGroundHeight(state.ground, pod.position.x,
                                             pod.position.z, pod.position.y);
    recovery.voidTime =
        (!pod.grounded && overVoid) ? recovery.voidTime + dt : 0.f;
    // Circling against a bank too steep for the height-field physics
    // still moves the pod, so progress along the lap is watched too.
    const int point = state.race.segment;
    const int count = static_cast<int>(state.lapPoints.size());
    const bool racing = state.race.countdown <= 0.f && !state.race.finished;
    // Only moving past the furthest point reached counts as progress;
    // circling back and forth over the same few points does not.
    const int gained = count > 0 && recovery.bestPoint >= 0
                           ? (point - recovery.bestPoint + count) % count
                           : 1;
    if (point >= 0 && gained > 0 && gained < count / 2) {
        recovery.bestPoint = point;
        recovery.stallTime = 0.f;
    } else if (racing) {
        recovery.stallTime += dt;
    }
    const bool crawling = throttle > 0.5f && pod.speed < 3.f;
    RacerRecoveryReason reason = RacerRecoveryReason::None;
    if (recovery.voidTime > kVoidSeconds || pod.airTime > kFallSeconds) {
        reason = RacerRecoveryReason::OffCourse;
    } else if (pod.stuckTime > kStuckSeconds && (pod.blocked || crawling)) {
        reason = RacerRecoveryReason::Stuck;
    } else if (recovery.stallTime > kStallSeconds) {
        reason = RacerRecoveryReason::NoProgress;
    }
    if (reason != RacerRecoveryReason::None) recovery = RacerRecovery{};
    return reason;
}

const char* RacerRecoveryName(RacerRecoveryReason reason) {
    switch (reason) {
    case RacerRecoveryReason::OffCourse: return "off course";
    case RacerRecoveryReason::Stuck: return "stuck";
    case RacerRecoveryReason::NoProgress: return "no progress";
    case RacerRecoveryReason::None: break;
    }
    return "none";
}

}  // namespace sdl3cpp::services::impl
