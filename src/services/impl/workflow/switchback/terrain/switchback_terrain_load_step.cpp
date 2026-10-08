#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_load_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_mesh.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_upload.hpp"
#include "services/interfaces/workflow/switchback/track/switchback_track_generate.hpp"
#include "services/interfaces/workflow/switchback/track/switchback_track_spec.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSwitchbackTerrainLoadStep::WorkflowSwitchbackTerrainLoadStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<SwitchbackTerrainState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowSwitchbackTerrainLoadStep::GetPluginId() const {
    return "switchback.terrain.load";
}

void WorkflowSwitchbackTerrainLoadStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_ || state_->attempted) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || !world) return;
    state_->attempted = true;

    const std::string path = Gta5ResolvePath(
        step, context, "track", "packages/switchback/tracks/spiral_pass.json");
    SwitchbackTrackSpec spec;
    if (!LoadSwitchbackTrackSpec(path, spec)) {
        LogError("cannot read track " + path);
        return;
    }
    SwitchbackTrackLayout layout;
    if (!GenerateSwitchbackTrack(spec, layout)) {
        LogError("track has no route: " + path);
        return;
    }
    const int cells = Gta5ParameterOrInt(step, "chunk_cells", 128);
    if (cells <= 0 || (layout.heightmap.size - 1) % cells != 0) {
        LogError("chunk_cells " + std::to_string(cells) + " does not divide " +
                 std::to_string(layout.heightmap.size - 1));
        return;
    }
    SwitchbackChunkSpec base;
    base.cells = cells;
    base.stepM = layout.stepM;
    base.uvMetres = Gta5NumberOr(step, "uv_metres", 20.f);
    try {
        UploadSwitchbackChunks(device, layout.heightmap, base, *state_);
    } catch (const std::runtime_error& error) {
        ReleaseSwitchbackChunks(device, *state_);
        LogError(error.what());
        return;
    }
    BuildSwitchbackCollision(*world, layout.heightmap, layout.stepM,
                             layout.heightMaxM, *state_);
    state_->checkpoints = std::move(layout.checkpoints);
    if (logger_) {
        logger_->Trace(
            "WorkflowSwitchbackTerrainLoadStep", "Execute",
            "samples=" + std::to_string(layout.heightmap.size) +
                ", chunks=" + std::to_string(state_->chunks.size()) +
                ", road_m=" + std::to_string(layout.roadLengthM) +
                ", grade_pct=" + std::to_string(layout.maxGradePercent) +
                ", checkpoints=" + std::to_string(state_->checkpoints.size()),
            "Terrain generated from " + path);
    }
}

void WorkflowSwitchbackTerrainLoadStep::LogError(const std::string& message) {
    if (logger_) {
        logger_->Error("switchback.terrain.load: " + message);
    }
}

}  // namespace sdl3cpp::services::impl
