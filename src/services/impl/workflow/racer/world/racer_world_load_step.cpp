#include "services/interfaces/workflow/racer/world/racer_world_load_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kDefaultTable =
    "packages/racer/assets/racer_tracks.json";

}  // namespace

WorkflowRacerWorldLoadStep::WorkflowRacerWorldLoadStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerWorldLoadStep::GetPluginId() const {
    return "racer.world.load";
}

void WorkflowRacerWorldLoadStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (state_->loaded) return;
    const std::string dir =
        RacerStringParam(step, "racer_dir", "RACER_DIR", "");
    state_->library = OpenRacerAssetLibrary(dir);
    const RacerTrackTable table = LoadRacerTrackTable(
        RacerStringParam(step, "track_table", nullptr, kDefaultTable));
    const std::string trackKey =
        RacerStringParam(step, "track", "RACER_TRACK", "1");
    const std::string podKey =
        RacerStringParam(step, "racer", "RACER_POD", "Anakin");
    const RacerTrackInfo* track = FindRacerTrack(table, trackKey);
    const RacerPodInfo* pod = FindRacerPod(table, podKey);
    if (!state_->library.valid || !track || !pod) {
        if (logger_) {
            logger_->Error("racer.world.load: need RACER_DIR (an Episode I "
                           "Racer install), a track and a racer; got '" +
                           dir + "', '" + trackKey + "', '" + podKey + "'");
        }
        return;
    }
    state_->track = *track;
    state_->racer = *pod;
    state_->textureScale = RacerIntParam(step, "texture_scale", 4);
    state_->race = RacerRaceState{};
    state_->race.lapsTotal = RacerIntParam(step, "laps", 3);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    state_->loaded = BuildRacerWorld(device, *state_, logger_);
    context.Set("racer.track_name", state_->track.name);
    context.Set("racer.pod_name", state_->racer.name);
}

}  // namespace sdl3cpp::services::impl
