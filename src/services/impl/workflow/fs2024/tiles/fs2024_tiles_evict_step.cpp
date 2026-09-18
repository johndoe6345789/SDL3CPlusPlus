#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_evict_step.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_release.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024TilesEvictStep::WorkflowFs2024TilesEvictStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TilesEvictStep::GetPluginId() const {
    return "fs2024.tiles.evict";
}

void WorkflowFs2024TilesEvictStep::Execute(const WorkflowStepDefinition&,
                                           WorkflowContext& context) {
    if (state_->pendingEvict.empty()) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    for (const Fs2024TileKey& key : state_->pendingEvict) {
        const auto it = state_->resident.find(key);
        if (it == state_->resident.end()) continue;
        ReleaseFs2024Tile(device, world, it->second);
        state_->resident.erase(it);
    }
    ReleaseUnusedFs2024LandmarkKits(device, *state_);
    if (logger_ && !state_->pendingEvict.empty()) {
        logger_->Trace("fs2024.tiles.evict: released " +
                       std::to_string(state_->pendingEvict.size()) +
                       " tiles, " + std::to_string(state_->resident.size()) +
                       " resident");
    }
    state_->pendingEvict.clear();
}

WorkflowFs2024TilesFreeStep::WorkflowFs2024TilesFreeStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TilesFreeStep::GetPluginId() const {
    return "fs2024.tiles.free";
}

void WorkflowFs2024TilesFreeStep::Execute(const WorkflowStepDefinition&,
                                          WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const std::size_t count = state_->resident.size();
    ReleaseAllFs2024Tiles(device, world, *state_);
    state_->pool.reset();  // joins the loaders, which read the world
    ReleaseUnusedFs2024LandmarkKits(device, *state_);  // now every kit
    if (state_->world && device) {
        SDL_ReleaseGPUTexture(device, state_->world->materialArray);
        SDL_ReleaseGPUSampler(device, state_->world->materialSampler);
        SDL_ReleaseGPUTexture(device, state_->world->roadTexture);
        SDL_ReleaseGPUSampler(device, state_->world->roadSampler);
    }
    state_->world.reset();
    if (logger_) {
        logger_->Info("fs2024.tiles.free: released " +
                      std::to_string(count) + " tiles");
    }
}

}  // namespace sdl3cpp::services::impl
