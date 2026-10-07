#include "racer_flow_setup.hpp"

#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

enum CupRow { kCircuit, kCupTrack, kCupRacer, kSplit, kCupStart, kCupBack };

}  // namespace

void UpdateRacerTournament(RacerFlow& flow, const RacerNav& nav,
                           const RacerTrackTable& table) {
    const int step = MoveSetupRow(flow, nav);
    const RacerProfile& profile = flow.profile;
    const int circuits = flow.unlockAll ? kRacerCircuits : profile.circuitsOpen;
    if (flow.setupRow == kCircuit && step != 0) {
        flow.circuit = (flow.circuit + step + circuits) % circuits;
        flow.circuitTrack = 0;
    }
    const int open = flow.unlockAll ? RacerCircuitTrackCount(flow.circuit)
                                    : profile.tracksOpen[flow.circuit];
    if (flow.setupRow == kCupTrack && step != 0) {
        flow.circuitTrack = (flow.circuitTrack + step + open) % open;
    }
    const int racers = static_cast<int>(table.racers.size());
    if (flow.setupRow == kCupRacer && step != 0) {
        flow.racerIndex = RacerStepOpen(flow.racerIndex, step, racers,
                                        OpenRacerList(flow, racers));
    }
    if (flow.setupRow == kSplit && step != 0) {
        flow.split = static_cast<RacerPurseSplit>(
            (static_cast<int>(flow.split) + step + 3) % 3);
    }
    if (!nav.select) return;
    if (flow.setupRow == kCupBack) {
        flow.phase = RacerPhase::Menu;
        return;
    }
    const int index =
        RacerCircuitTrackIndex(table, flow.circuit, flow.circuitTrack);
    if (index < 0) return;
    flow.trackIndex = index;
    flow.laps = 3;
    flow.tournament = true;
    StartRacerLoad(flow);
    // The planet's flyover first, the first time the tournament goes there.
    QueueRacerPlanetIntro(flow, table.tracks[index].planet);
}

}  // namespace sdl3cpp::services::impl
