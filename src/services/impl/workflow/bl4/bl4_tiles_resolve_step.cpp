#include "services/interfaces/workflow/bl4/bl4_tiles_resolve_step.hpp"

#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"
#include "services/interfaces/workflow/bl4/bl4_tiles_resolve.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowBl4TilesResolveStep::WorkflowBl4TilesResolveStep(std::shared_ptr<ILogger> logger,
                                                        std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4TilesResolveStep::GetPluginId() const { return "bl4.tiles.resolve"; }

void WorkflowBl4TilesResolveStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (!state_->configured) {
        state_->mapRoot = Bl4StringOr(step, "map_root", "");
        state_->tileSize = Bl4NumberOrEnv(step, "tile_size", 64.f);
        state_->loadRadiusTiles = static_cast<int>(Bl4NumberOrEnv(step, "load_radius_tiles", 2.f));
        state_->evictRadiusTiles = static_cast<int>(Bl4NumberOrEnv(step, "evict_radius_tiles", 3.f));
        state_->maxLoadsPerCall = static_cast<int>(Bl4NumberOrEnv(step, "max_loads_per_call", 4.f));
        state_->configured = true;
        if (logger_) {
            logger_->Info("bl4.tiles.resolve: streaming '" + state_->mapRoot + "', " +
                          std::to_string(state_->tileSize) + " m tiles, load radius " +
                          std::to_string(state_->loadRadiusTiles) + ", evict radius " +
                          std::to_string(state_->evictRadiusTiles));
        }
    }
    const auto* player = context.TryGet<Q3PlayerState>("q3.ps");
    if (!player) return;
    Bl4ResolveWantedTiles(*state_, player->origin.x, player->origin.z);
}

}  // namespace sdl3cpp::services::impl
