#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

#include <algorithm>
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
        if (state_->world) state_->tileSize = state_->world->origin.TileSize();
        state_->lod.rootRadius =
            static_cast<int>(Fs2024NumberOr(step, "root_radius", 2.f));
        state_->lod.split = Fs2024NumberOr(step, "split", 1.f);
        state_->leadSeconds = Fs2024NumberOr(step, "lead_seconds", 1.5f);
        state_->finishBudgetMs = Fs2024NumberOr(step, "finish_budget_ms", 4.f);
        state_->configured = true;
        if (logger_) {
            logger_->Info("fs2024.tiles.resolve: " +
                          std::to_string(state_->tileSize) +
                          " m finest tiles, " +
                          std::to_string(state_->lod.rootRadius) +
                          " coarsest tiles out, split " +
                          std::to_string(state_->lod.split));
        }
    }
    const auto* player = context.TryGet<Q3PlayerState>("q3.ps");
    if (!player) return;
    // Stream where the camera will be, and let height above the ground
    // coarsen everything below it.
    const glm::vec3 eye =
        context.Get<glm::vec3>("render.camera_pos", player->origin);
    glm::vec3 viewer = eye + player->velocity * state_->leadSeconds;
    const Fs2024Heightfield* field = Fs2024FindTileField(*state_, eye.x, eye.z);
    const float ground = field ? Fs2024HeightAt(*field, eye.x, eye.z) : 0.f;
    viewer.y = std::max(eye.y - ground, 0.f);
    Fs2024ResolveWantedTiles(*state_, viewer);
}

}  // namespace sdl3cpp::services::impl
