#include "services/interfaces/workflow/fs2024/fs2024_terrain_free_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_terrain_upload.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024TerrainFreeStep::WorkflowFs2024TerrainFreeStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TerrainState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TerrainFreeStep::GetPluginId() const {
    return "fs2024.terrain.free";
}

void WorkflowFs2024TerrainFreeStep::Execute(const WorkflowStepDefinition&,
                                            WorkflowContext& context) {
    const std::size_t blocks = state_->chunks.size();
    ReleaseFs2024TerrainChunks(
        context.Get<SDL_GPUDevice*>("gpu_device", nullptr), *state_);
    RemoveFs2024TerrainCollision(
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr),
        state_->collision);
    // Only after the shape is gone: it was reading these heights.
    state_->field = Fs2024Heightfield{};
    state_->loaded = false;
    context.Set<bool>("fs2024.terrain.loaded", false);
    if (logger_) {
        logger_->Info("fs2024.terrain.free: released " +
                      std::to_string(blocks) + " blocks");
    }
}

}  // namespace sdl3cpp::services::impl
