#include "services/interfaces/workflow/racer/world/racer_race_setup.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"
#include "services/interfaces/workflow/racer/world/racer_field_build.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>
#include <cstdlib>

namespace sdl3cpp::services::impl {
namespace {

std::string Env(const char* name) {
    const char* value = std::getenv(name);
    return value ? value : "";
}

int IndexOf(const std::vector<RacerTrackInfo>& tracks,
            const RacerTrackInfo* track) {
    return track ? static_cast<int>(track - tracks.data()) : 1;
}

}  // namespace

void InitRacerFlow(RacerWorldState& state, const RacerSetupPaths& paths,
                   const std::shared_ptr<ILogger>& logger) {
    RacerFlow& flow = state.flow;
    flow.initialised = true;
    state.textureScale = paths.textureScale;
    state.library = OpenRacerAssetLibrary(paths.racerDir);
    state.table = LoadRacerTrackTable(paths.trackTable);
    flow.profile = LoadRacerProfile(RacerProfilePath());
    if (!state.library.valid && logger) {
        logger->Error("racer: RACER_DIR '" + paths.racerDir +
                      "' is not an Episode I Racer install");
        flow.notice = "SET RACER_DIR TO YOUR EPISODE I RACER FOLDER";
    }
    if (const std::string laps = Env("RACER_LAPS"); !laps.empty()) {
        flow.laps = std::max(1, std::atoi(laps.c_str()));
    }
    const std::string track = Env("RACER_TRACK");
    const std::string pod = Env("RACER_POD");
    if (track.empty() && pod.empty() && Env("RACER_AUTOPILOT").empty()) {
        flow.phase = RacerPhase::Menu;
        return;
    }
    flow.trackIndex = IndexOf(state.table.tracks,
                              FindRacerTrack(state.table, track));
    if (const RacerPodInfo* racer = FindRacerPod(state.table, pod)) {
        flow.racerIndex = static_cast<int>(racer - state.table.racers.data());
    }
    flow.phase = RacerPhase::Loading;
    flow.requestLoad = true;
}

}  // namespace sdl3cpp::services::impl
