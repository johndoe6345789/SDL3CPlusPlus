#include "services/interfaces/workflow/racer/world/racer_world_load_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"
#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"
#include "services/interfaces/workflow/racer/world/racer_race_setup.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kDefaultTable =
    "packages/racer/assets/racer_tracks.json";
constexpr int kLoadingFrames = 3;  // let the loading screen show first

}  // namespace

WorkflowRacerWorldLoadStep::WorkflowRacerWorldLoadStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerWorldLoadStep::GetPluginId() const {
    return "racer.world.load";
}

void WorkflowRacerWorldLoadStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    RacerFlow& flow = state_->flow;
    if (!flow.initialised) {
        RacerSetupPaths paths;
        paths.racerDir = RacerStringParam(step, "racer_dir", "RACER_DIR", "");
        paths.trackTable =
            RacerStringParam(step, "track_table", nullptr, kDefaultTable);
        paths.textureScale = RacerIntParam(step, "texture_scale", 4);
        flow.laps = RacerIntParam(step, "laps", flow.laps);
        flow.opponents = RacerIntParam(step, "opponents", flow.opponents);
        InitRacerFlow(*state_, paths, logger_);
    }
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (flow.requestRelease) {
        flow.requestRelease = false;
        ReleaseRacerWorld(device, *state_);
    }
    if (!flow.requestLoad || flow.phase != RacerPhase::Loading) return;
    if (++flow.loadingFrames < kLoadingFrames) return;
    flow.requestLoad = false;
    if (LoadRacerRace(device, *state_, logger_)) {
        context.Set("racer.track_name", state_->track.name);
        context.Set("racer.pod_name", state_->racer.name);
    }
}

}  // namespace sdl3cpp::services::impl
