#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"

#include "services/interfaces/workflow/racer/player/racer_surface_effects.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kFallSeconds = 5.f;    // airborne this long: off course
constexpr float kVoidSeconds = 1.f;    // over nothing this long: off
constexpr float kFallDepth = 12.f;     // metres under the line: fallen
constexpr float kStuckSeconds = 2.f;   // pinned this long: put back
constexpr float kStallSeconds = 6.f;   // no lap progress this long

}  // namespace

RacerRecoveryReason UpdateRacerRecovery(RacerRecovery& recovery,
                                        const RacerPodState& pod,
                                        const RacerRaceState& race,
                                        const RacerGround& ground,
                                        const std::vector<glm::vec3>& lap,
                                        float throttle, float dt) {
    const int lapPointCount = static_cast<int>(lap.size());
    const bool overVoid = !RacerGroundHeight(ground, pod.position.x,
                                             pod.position.z, pod.position.y);
    // Jumps cross gaps with nothing below for seconds; only a pod that
    // has dropped well below the racing line has fallen off.
    const bool belowLine =
        race.segment < 0 || lap.empty() ||
        pod.position.y < lap[race.segment % lapPointCount].y - kFallDepth;
    recovery.voidTime = (!pod.grounded && overVoid && belowLine)
                            ? recovery.voidTime + dt
                            : 0.f;
    // Circling against a bank too steep for the height-field physics
    // still moves the pod, so progress along the lap is watched too.
    const int point = race.segment;
    const int count = lapPointCount;
    const bool racing = race.countdown <= 0.f && !race.finished;
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
    const bool fatal = RacerSurfaceEffectFor(pod.surface).fatal;
    if (fatal || recovery.voidTime > kVoidSeconds ||
        pod.airTime > kFallSeconds) {
        reason = RacerRecoveryReason::OffCourse;
    } else if (pod.stuckTime > kStuckSeconds && (pod.blocked || crawling)) {
        reason = RacerRecoveryReason::Stuck;
    } else if (recovery.stallTime > kStallSeconds) {
        reason = RacerRecoveryReason::NoProgress;
    }
    if (reason != RacerRecoveryReason::None) {
        RacerRecovery fresh;  // the timers restart; the streak carries on
        fresh.lastRespawn = recovery.lastRespawn;
        fresh.streak = recovery.streak;
        recovery = fresh;
    }
    return reason;
}

}  // namespace sdl3cpp::services::impl
