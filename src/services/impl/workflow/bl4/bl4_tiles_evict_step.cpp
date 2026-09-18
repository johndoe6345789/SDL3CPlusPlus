#include "services/interfaces/workflow/bl4/bl4_tiles_evict_step.hpp"

#include "services/interfaces/workflow/bl4/bl4_collision_body.hpp"
#include "services/interfaces/workflow/bl4/bl4_collision_shape.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

void ReleaseInstance(SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                     Bl4TileStreamState& state, Bl4Instance& instance) {
    RemoveBl4InstanceBody(world, instance);
    Bl4Geometry* geometry = instance.geometry;
    if (!geometry) return;
    if (--geometry->references > 0) return;

    if (device) {
        for (const Bl4SubMesh& sub : geometry->subMeshes) {
            SDL_ReleaseGPUBuffer(device, sub.vertexBuffer);
            SDL_ReleaseGPUBuffer(device, sub.indexBuffer);
        }
    }
    for (const Bl4SubMesh& sub : geometry->subMeshes) {
        ReleaseBl4Texture(state.textureCache, sub.texturePath, device);
    }
    ReleaseBl4CollisionShape(*geometry);
    state.geometryCache.erase(geometry->modelPath);
}

void ReleaseTile(SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                 Bl4TileStreamState& state, Bl4LoadedTile& tile) {
    for (Bl4Instance& instance : tile.instances) ReleaseInstance(device, world, state, instance);
    for (std::uint64_t identity : tile.owned) state.liveInstances.erase(identity);
    tile.owned.clear();
}

}  // namespace

WorkflowBl4TilesEvictStep::WorkflowBl4TilesEvictStep(std::shared_ptr<ILogger> logger,
                                                    std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4TilesEvictStep::GetPluginId() const { return "bl4.tiles.evict"; }

void WorkflowBl4TilesEvictStep::Execute(const WorkflowStepDefinition&, WorkflowContext& context) {
    if (state_->pendingEvict.empty()) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    for (const Bl4TileKey& key : state_->pendingEvict) {
        const auto it = state_->resident.find(key);
        if (it == state_->resident.end()) continue;
        ReleaseTile(device, world, *state_, it->second);
        state_->resident.erase(it);
    }
    if (logger_ && !state_->pendingEvict.empty()) {
        logger_->Trace("bl4.tiles.evict: released " +
                       std::to_string(state_->pendingEvict.size()) + " tiles, " +
                       std::to_string(state_->resident.size()) + " resident");
    }
    state_->pendingEvict.clear();
}

WorkflowBl4TilesFreeStep::WorkflowBl4TilesFreeStep(std::shared_ptr<ILogger> logger,
                                                  std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4TilesFreeStep::GetPluginId() const { return "bl4.tiles.free"; }

void WorkflowBl4TilesFreeStep::Execute(const WorkflowStepDefinition&, WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const std::size_t count = state_->resident.size();
    for (auto& [key, tile] : state_->resident) ReleaseTile(device, world, *state_, tile);
    state_->resident.clear();
    state_->pendingLoad.clear();
    state_->pendingEvict.clear();
    state_->missing.clear();
    state_->liveInstances.clear();
    state_->textureCache.clear();  // only failed-load entries remain
    ReleaseBl4InstanceBatch(device, state_->batch);
    if (logger_) logger_->Info("bl4.tiles.free: released " + std::to_string(count) + " tiles");
}

}  // namespace sdl3cpp::services::impl
