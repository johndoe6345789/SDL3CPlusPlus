#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve_step.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024TilesResolveStep::WorkflowFs2024TilesResolveStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TilesResolveStep::GetPluginId() const {
    return "fs2024.tiles.resolve";
}

void WorkflowFs2024TilesResolveStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_->configured) {
        // Tile size is FS2024's own level-14 cut, fixed by where
        // fs2024.world.open put engine space on the Earth.
        if (state_->world) state_->tileSize = state_->world->origin.TileSize();
        state_->loadRadiusTiles = static_cast<int>(
            Fs2024NumberOr(step, "load_radius_tiles", 2.f));
        state_->evictRadiusTiles = static_cast<int>(
            Fs2024NumberOr(step, "evict_radius_tiles", 3.f));
        state_->maxLoadsPerCall = static_cast<int>(
            Fs2024NumberOr(step, "max_loads_per_call", 4.f));
        state_->configured = true;
        if (logger_) {
            logger_->Info("fs2024.tiles.resolve: " +
                          std::to_string(state_->tileSize) +
                          " m tiles, load radius " +
                          std::to_string(state_->loadRadiusTiles) +
                          ", evict radius " +
                          std::to_string(state_->evictRadiusTiles));
        }
    }
    const auto* player = context.TryGet<Q3PlayerState>("q3.ps");
    if (!player) return;
    Fs2024ResolveWantedTiles(*state_, player->origin.x, player->origin.z);
}

}  // namespace sdl3cpp::services::impl
