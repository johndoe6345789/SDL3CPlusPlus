#include "services/interfaces/workflow/racer/player/racer_opponents_step.hpp"

#include "services/interfaces/workflow/racer/player/racer_autopilot.hpp"
#include "services/interfaces/workflow/racer/player/racer_lap_progress.hpp"
#include "services/interfaces/workflow/racer/player/racer_line_guide.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_report.hpp"
#include "services/interfaces/workflow/racer/player/racer_traffic.hpp"
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
        input = RacerAutopilot(opponent.pod, state.lapPoints, race.segment,
                               opponent.spec);
        std::vector<const RacerPodState*> others{&state.pod};
        for (const RacerOpponent& rival : state.opponents) {
            others.push_back(&rival.pod);
        }
        AvoidRacerTraffic(opponent.pod, others, input);
    }
    StepRacerPod(opponent.pod, input, opponent.spec, dt,
                 RacerGroundSurface(state.ground));
    GuideRacerPodToLine(opponent.pod, state.lapPoints, race.segment,
                        kRacerAiLineOffset, kRacerAiLinePull, dt);
    opponent.roll +=
        (input.steer * 0.45f - opponent.roll) * std::min(1.f, 6.f * dt);
    RacerPodState& pod = opponent.pod;
    const bool crawling = input.throttle > 0.5f && pod.speed < 3.f;
    pod.stuckTime = (pod.blocked || crawling) ? pod.stuckTime + dt : 0.f;
    const RacerRecoveryReason reason = UpdateRacerRecovery(
        opponent.recovery, pod, race, state.ground, state.lapPoints,
        input.throttle, dt);
    if (reason == RacerRecoveryReason::None) return;
    const int respawnAt = RacerRespawnPoint(opponent.recovery, race.segment,
                                            reason, kRacerLapSamples);
    PlaceRacerPod(state, pod, respawnAt, 0.3f * opponent.spec.topSpeed, 0.f);
}

}  // namespace sdl3cpp::services::impl
