#include "services/interfaces/workflow/racer/player/racer_opponents_step.hpp"

#include "services/interfaces/workflow/racer/player/racer_autopilot.hpp"
#include "services/interfaces/workflow/racer/player/racer_lap_progress.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_report.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void FlyRacerOpponent(RacerWorldState& state, RacerOpponent& opponent,
                      float dt) {
    const int count = static_cast<int>(state.lapPoints.size());
    RacerRaceState& race = opponent.race;
    const int point =
        NearestRacerLapPoint(state.lapPoints, opponent.pod.position,
                             race.segment);
    AdvanceRacerRace(race, point, count, dt);
    RacerPodInput input;
    if (race.countdown <= 0.f && !race.finished) {
        input = RacerAutopilot(opponent.pod, state.lapPoints, race.segment);
        // The autopilot plans for a stock pod; a slower pod eases off.
        input.throttle *= std::min(1.f, opponent.spec.topSpeed /
                                            state.podSpec.topSpeed + 0.05f);
    }
    StepRacerPod(opponent.pod, input, opponent.spec, dt,
                 RacerGroundSurface(state.ground));
    opponent.roll +=
        (input.steer * 0.45f - opponent.roll) * std::min(1.f, 6.f * dt);
    RacerPodState& pod = opponent.pod;
    const bool crawling = input.throttle > 0.5f && pod.speed < 3.f;
    pod.stuckTime = (pod.blocked || crawling) ? pod.stuckTime + dt : 0.f;
    const RacerRecoveryReason reason = UpdateRacerRecovery(
        opponent.recovery, pod, race, state.ground, count, input.throttle,
        dt);
    if (reason == RacerRecoveryReason::None) return;
    const int ahead =
        reason == RacerRecoveryReason::NoProgress ? 2 * kRacerLapSamples : 0;
    PlaceRacerPod(state, pod, std::max(0, race.segment) + ahead,
                  0.3f * opponent.spec.topSpeed, 0.f);
}

}  // namespace sdl3cpp::services::impl
