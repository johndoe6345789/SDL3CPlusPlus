#include "services/interfaces/workflow/racer/world/racer_race_setup.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_handling.hpp"
#include "services/interfaces/workflow/racer/world/racer_field_build.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

namespace sdl3cpp::services::impl {

bool LoadRacerRace(SDL_GPUDevice* device, RacerWorldState& state,
                   const std::shared_ptr<ILogger>& logger) {
    RacerFlow& flow = state.flow;
    const auto& table = state.table;
    if (!state.library.valid ||
        flow.trackIndex >= static_cast<int>(table.tracks.size()) ||
        flow.racerIndex >= static_cast<int>(table.racers.size())) {
        flow.phase = RacerPhase::Menu;
        return false;
    }
    state.track = table.tracks[flow.trackIndex];
    state.racer = table.racers[flow.racerIndex];
    if (const auto fog = table.fog.find(state.track.planet);
        fog != table.fog.end()) {
        state.fogColour = {fog->second[0], fog->second[1], fog->second[2]};
    }
    if (logger) {
        logger->Trace("racer.world.load: " + state.track.planet + " fog " +
                      std::to_string(state.fogColour.r) + " " +
                      std::to_string(state.fogColour.g) + " " +
                      std::to_string(state.fogColour.b));
    }
    // The tournament pod carries the parts bought for it; a free race
    // is run on the racer's stock pod, as in the original.
    state.podSpec = RacerSpecFromHandling(state.racer.handling);
    if (flow.tournament) {
        state.podSpec = ApplyRacerUpgrades(state.podSpec, flow.profile);
    }
    state.race = RacerRaceState{};
    state.race.lapsTotal = flow.laps;
    state.loaded = BuildRacerWorld(device, state, logger);
    if (!state.loaded) {
        flow.phase = RacerPhase::Menu;
        flow.notice = "THAT TRACK WOULD NOT LOAD";
        return false;
    }
    BuildRacerField(device, state, table, flow.opponents, logger);
    flow.phase = RacerPhase::Racing;
    flow.resultsDelay = 0.f;
    flow.prizeAwarded = false;
    flow.prize = 0;
    return true;
}

}  // namespace sdl3cpp::services::impl
