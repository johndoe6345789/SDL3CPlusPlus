#include "services/interfaces/workflow/gta5/gta5_tiles_load_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_grid.hpp"
#include "services/interfaces/workflow/gta5/gta5_spawn_tile.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_io.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_order.hpp"
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

    const std::string tilesDir = Gta5ResolvePath(
        step, context, "tiles_dir", "packages/gta5/assets/tiles");

    int budget = Gta5ParameterOrInt(step, "max_spawns_per_frame",
                                    state_->streaming.maxSpawnsPerFrame);
    if (budget <= 0) budget = 64;

    int spawned = 0;
    for (const Gta5TileCoord& tile : OrderGta5TilesByDistance(*state_)) {
        if (spawned >= budget) break;
        Gta5ResidentTile& resident = state_->resident[tile];

        if (!resident.placementsRead) {
            // A missing tile file is normal out at sea. Mark it read so we
            // do not stat the same absent file every frame.
            ReadGta5TileFile(Gta5TilePath(tilesDir, tile),
                             resident.placements, logger_);
            resident.placementsRead = true;
            const float distance =
                glm::distance(Gta5TileCentre(state_->world, tile),
                              state_->centreOrigin);
            resident.bandAtSpawn =
                Gta5BandForDistance(state_->world, distance);
            if (logger_ && !resident.placements.empty()) {
                logger_->Info("gta5.tiles.load: tile " +
                              std::to_string(tile.x) + "_" +
                              std::to_string(tile.z) + " has " +
                              std::to_string(resident.placements.size()) +
                              " placements");
            }
        }

        spawned += SpawnGta5TilePlacements(
            *state_, resident, budget - spawned, device, world, logger_);
    }

    if (spawned > 0) {
        context.Set("gta5.tiles.spawned_last_frame", spawned);
    }
}

}  // namespace sdl3cpp::services::impl
