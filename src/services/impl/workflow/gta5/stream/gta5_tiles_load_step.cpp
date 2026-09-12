#include "services/interfaces/workflow/gta5/stream/gta5_tiles_load_step.hpp"

#include "services/interfaces/workflow/gta5/resource/gta5_assets_index_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_geometry_request.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_load_budget.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_prepared_finish.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_spawn_tile.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_tile_order.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_tile_source.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5TilesLoadStep::WorkflowGta5TilesLoadStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5TilesLoadStep::GetPluginId() const {
    return "gta5.tiles.load";
}

void WorkflowGta5TilesLoadStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (!state_ || state_->wanted.empty()) return;

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) return;
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    // With the map indexed, tiles are read from its ymaps; until the
    // background build finishes there is nothing to load yet.
    const bool indexed = state_->assets || state_->assetsPending.valid();
    if (indexed && !Gta5AssetsReady(*state_, logger_)) return;

    const Gta5CostTimer whole(state_->cost.load);
    const Gta5LoadBudget limits =
        ReadGta5LoadBudget(step, context, state_->streaming);
    {  // First, upload what the workers have finished, within a budget.
        const Gta5CostTimer finishing(state_->cost.finish);
        FinishGta5PreparedGeometry(*state_, device, limits, logger_);
    }

    const std::string tilesDir = Gta5ResolvePath(
        step, context, "tiles_dir", "packages/gta5/assets/tiles");
    const int budget = limits.spawns;
    int reads = limits.reads;

    int spawned = 0;
    for (const Gta5TileCoord& tile : OrderGta5TilesByDistance(*state_)) {
        if (spawned >= budget) break;
        Gta5ResidentTile& resident = state_->resident[tile];
        if (!resident.placementsRead) {
            if (!resident.reading.valid()) {
                if (reads-- > 0) {
                    StartGta5TileRead(*state_, tilesDir, tile, resident,
                                      logger_);
                }
                continue;
            }
            const Gta5CostTimer adopting(state_->cost.adopt);
            if (!TakeGta5TileRead(*state_, tile, resident, logger_)) continue;
        } else if (!resident.prefetched) {
            PrefetchGta5Tile(*state_, resident);  // rebuilt at a new band
        }
        const Gta5CostTimer spawning(state_->cost.spawn);
        spawned += SpawnGta5TilePlacements(
            *state_, resident, budget - spawned, device, world, logger_);
    }

    if (spawned > 0) context.Set("gta5.tiles.spawned_last_frame", spawned);
    state_->cost.spawned += spawned;
}

}  // namespace sdl3cpp::services::impl
