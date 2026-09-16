#include "services/interfaces/workflow/fs2024/fs2024_terrain_load_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_upload.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024TerrainLoadStep::WorkflowFs2024TerrainLoadStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TerrainState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TerrainLoadStep::GetPluginId() const {
    return "fs2024.terrain.load";
}

void WorkflowFs2024TerrainLoadStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    const std::string path = Fs2024StringOr(step, "file_path", "");
    if (path.empty()) {
        throw std::runtime_error("fs2024.terrain.load: file_path is required");
    }
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("fs2024.terrain.load: no GPU device");
    }
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    // A second load replaces the first rather than leaking it.
    ReleaseFs2024TerrainChunks(device, *state_);
    RemoveFs2024TerrainCollision(world, state_->collision);

    state_->field = ReadFs2024Heightfield(path);
    const int cells = static_cast<int>(
        Fs2024NumberOr(step, "cells_per_chunk", 64.f));
    const std::size_t triangles =
        UploadFs2024TerrainChunks(device, *state_, cells > 0 ? cells : 64);
    state_->collision = AddFs2024TerrainCollision(world, state_->field);
    state_->loaded = true;
    context.Set<bool>("fs2024.terrain.loaded", true);

    if (logger_) {
        const Fs2024Heightfield& field = state_->field;
        logger_->Info(
            "fs2024.terrain.load: " + path + " " +
            std::to_string(field.columns) + "x" +
            std::to_string(field.rows) + " @ " +
            std::to_string(field.spacing) + " m, heights " +
            std::to_string(field.minHeight) + ".." +
            std::to_string(field.maxHeight) + ", " +
            std::to_string(state_->chunks.size()) + " blocks, " +
            std::to_string(triangles) + " triangles, collision " +
            (world ? "on" : "off (no physics_world)"));
    }
}

}  // namespace sdl3cpp::services::impl
