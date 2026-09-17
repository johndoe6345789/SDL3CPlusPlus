#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_evict_step.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_load.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_upload.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

void ReleaseTile(SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                 Fs2024LoadedTile& tile) {
    ReleaseFs2024TerrainChunks(device, tile.terrain);
    RemoveFs2024TerrainCollision(world, tile.terrain.collision);
    if (device) {
        SDL_ReleaseGPUTexture(device, tile.groundTexture);
        SDL_ReleaseGPUSampler(device, tile.groundSampler);
        SDL_ReleaseGPUBuffer(device, tile.buildingChunk.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, tile.buildingChunk.indexBuffer);
        SDL_ReleaseGPUBuffer(device, tile.buildingRoofChunk.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, tile.buildingRoofChunk.indexBuffer);
    }
}

}  // namespace

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
        ReleaseTile(device, world, it->second);
        state_->resident.erase(it);
    }
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
    for (auto& [key, tile] : state_->resident) {
        ReleaseTile(device, world, tile);
    }
    state_->resident.clear();
    state_->pendingLoad.clear();
    state_->pendingEvict.clear();
    state_->missing.clear();
    for (auto& [model, kit] : state_->landmarkKits) {
        ReleaseFs2024LandmarkKitGpu(device, kit);
    }
    state_->landmarkKits.clear();
    if (logger_) {
        logger_->Info("fs2024.tiles.free: released " +
                      std::to_string(count) + " tiles");
    }
}

}  // namespace sdl3cpp::services::impl
