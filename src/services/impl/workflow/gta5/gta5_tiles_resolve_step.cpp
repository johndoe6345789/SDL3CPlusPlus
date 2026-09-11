#include "services/interfaces/workflow/gta5/gta5_tiles_resolve_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_grid.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_lead.hpp"
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

    if (!state_->world.loaded) {
        LoadGta5WorldConfig(
            Gta5ResolvePath(step, context, "config",
                            "packages/gta5/config/gta5_world.json"),
            state_->world, logger_);
    }
    if (!state_->streaming.loaded) {
        LoadGta5StreamingConfig(
            Gta5ResolvePath(step, context, "streaming",
                            "packages/gta5/config/streaming.json"),
            state_->streaming, logger_);
    }

    const auto* playerState = context.TryGet<Q3PlayerState>("q3.ps");
    if (!playerState) {
        // Nothing to centre on yet. Leave the resident set alone rather
        // than evicting the whole map back to nothing.
        return;
    }

    const glm::vec3 origin = Gta5StreamOrigin(*state_, playerState->origin);
    state_->centreOrigin = origin;
    state_->centre = Gta5TileForPosition(state_->world, origin);
    ResolveGta5WantedTiles(
        *state_, origin,
        origin + Gta5StreamLead(*state_, playerState->velocity));

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
