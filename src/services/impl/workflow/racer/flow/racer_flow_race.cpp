#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

namespace sdl3cpp::services::impl {
namespace {

enum PauseRow { kResume, kRestart, kToMenu, kPauseRows };

constexpr float kResultsDelay = 3.f;  // the finish line, then results

void Restart(RacerFlow& flow) {
    flow.requestRelease = true;
    flow.requestLoad = true;
    flow.loadingFrames = 0;
    flow.phase = RacerPhase::Loading;
}

void ToMenu(RacerFlow& flow) {
    flow.requestRelease = true;
    flow.phase = RacerPhase::Menu;
}

/// Prize money once, when the results first show.
void Award(RacerFlow& flow, const RacerWorldState& state) {
    if (flow.prizeAwarded) return;
    flow.prizeAwarded = true;
    flow.prize = RacerPrizeFor(state.race.position);
    flow.profile.truguts += flow.prize;
    SaveRacerProfile(flow.profile, RacerProfilePath());
}

}  // namespace

void UpdateRacerRaceFlow(RacerFlow& flow, RacerWorldState& state,
                         const RacerNav& nav, float dt) {
    if (flow.phase == RacerPhase::Racing) {
        if (nav.back) {
            flow.phase = RacerPhase::Paused;
            flow.pauseRow = kResume;
        } else if (state.race.finished) {
            flow.resultsDelay += dt;
            if (flow.resultsDelay > kResultsDelay) {
                Award(flow, state);
                flow.phase = RacerPhase::Results;
            }
        }
        return;
    }
    if (flow.phase == RacerPhase::Paused) {
        if (nav.up) {
            flow.pauseRow = (flow.pauseRow + kPauseRows - 1) % kPauseRows;
        }
        if (nav.down) flow.pauseRow = (flow.pauseRow + 1) % kPauseRows;
        if (nav.back) flow.phase = RacerPhase::Racing;
        if (!nav.select) return;
        if (flow.pauseRow == kResume) flow.phase = RacerPhase::Racing;
        if (flow.pauseRow == kRestart) Restart(flow);
        if (flow.pauseRow == kToMenu) ToMenu(flow);
        return;
    }
    if (flow.phase == RacerPhase::Results && (nav.select || nav.back)) {
        ToMenu(flow);
    }
}

}  // namespace sdl3cpp::services::impl
