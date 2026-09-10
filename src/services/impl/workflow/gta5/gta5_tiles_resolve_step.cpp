#include "services/interfaces/workflow/gta5/gta5_tiles_resolve_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_grid.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_streaming_config_load.hpp"
#include "services/interfaces/workflow/gta5/gta5_wanted_tiles.hpp"
#include "services/interfaces/workflow/gta5/gta5_world_config_load.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5TilesResolveStep::WorkflowGta5TilesResolveStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5TilesResolveStep::GetPluginId() const {
    return "gta5.tiles.resolve";
}

void WorkflowGta5TilesResolveStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_) return;

    const std::string packageDir = context.Get<std::string>("package_dir", "");
    if (!state_->world.loaded) {
        LoadGta5WorldConfig(
            packageDir + "/" +
                Gta5ParameterOr(step, "config", "config/gta5_world.json"),
            state_->world, logger_);
    }
    if (!state_->streaming.loaded) {
        LoadGta5StreamingConfig(
            packageDir + "/" +
                Gta5ParameterOr(step, "streaming", "config/streaming.json"),
            state_->streaming, logger_);
    }

    const auto* playerState = context.TryGet<Q3PlayerState>("q3.ps");
    if (!playerState) {
        // Nothing to centre on yet. Leave the resident set alone rather
        // than evicting the whole map back to nothing.
        return;
    }

    glm::vec3 centre = playerState->origin;
    if (state_->streaming.prefetchEnabled) {
        centre += playerState->velocity * state_->streaming.velocityLeadSeconds;
    }
    state_->centreOrigin = playerState->origin;
    state_->centre = Gta5TileForPosition(state_->world, centre);
    ResolveGta5WantedTiles(*state_, centre);

    context.Set("gta5.tiles.wanted_count",
                static_cast<int>(state_->wanted.size()));
    context.Set("gta5.tiles.resident_count",
                static_cast<int>(state_->resident.size()));

    if (logger_) {
        logger_->Trace("WorkflowGta5TilesResolveStep", "Execute",
                       "centre=" + std::to_string(state_->centre.x) + "_" +
                           std::to_string(state_->centre.z),
                       "wanted=" + std::to_string(state_->wanted.size()));
    }
}

}  // namespace sdl3cpp::services::impl
