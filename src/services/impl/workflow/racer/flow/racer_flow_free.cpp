#include "racer_flow_setup.hpp"

#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

enum FreeRow { kTrack, kRacer, kLaps, kRivals, kFreeStart, kFreeBack };

}  // namespace

void UpdateRacerFreeRace(RacerFlow& flow, const RacerNav& nav,
                         const RacerTrackTable& table) {
    const int step = MoveSetupRow(flow, nav);
    const int tracks = static_cast<int>(table.tracks.size());
    const int racers = static_cast<int>(table.racers.size());
    std::vector<bool> openTracks(tracks);
    for (int i = 0; i < tracks; ++i) {
        openTracks[i] = RacerTrackOpen(flow, table, i);
    }
    if (step != 0 && flow.setupRow == kTrack) {
        flow.trackIndex = RacerStepOpen(flow.trackIndex, step, tracks,
                                        openTracks);
    }
    if (step != 0 && flow.setupRow == kRacer) {
        flow.racerIndex = RacerStepOpen(flow.racerIndex, step, racers,
                                        OpenRacerList(flow, racers));
    }
    if (flow.setupRow == kLaps) flow.laps = std::clamp(flow.laps + step, 1, 5);
    if (flow.setupRow == kRivals) {
        flow.opponents = std::clamp(flow.opponents + step, 0, 11);
    }
    if (!nav.select) return;
    if (flow.setupRow == kFreeBack) {
        flow.phase = RacerPhase::Menu;
    } else {
        flow.tournament = false;
        StartRacerLoad(flow);
    }
}

}  // namespace sdl3cpp::services::impl
