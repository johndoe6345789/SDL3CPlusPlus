#include "services/interfaces/workflow/gta5/gta5_lod_select_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_grid.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5LodSelectStep::WorkflowGta5LodSelectStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5LodSelectStep::GetPluginId() const {
    return "gta5.lod.select";
}

void WorkflowGta5LodSelectStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (!state_ || state_->resident.empty()) return;

    int changed = 0;
    for (const auto& entry : state_->resident) {
        const Gta5ResidentTile& resident = entry.second;

        // A tile still part-way through spawning is left alone. Re-banding
        // it now would throw away the objects it just paid for.
        if (resident.spawnedCount < resident.placements.size()) continue;

        const float distance =
            Gta5TileDistance(state_->world, entry.first, state_->centreOrigin);
        if (Gta5BandForDistance(state_->world, distance) !=
            resident.bandAtSpawn) {
            state_->rebuild.insert(entry.first);
            ++changed;
        }
    }

    context.Set("gta5.lod.rebuild_count", changed);
    if (changed > 0 && logger_) {
        logger_->Trace("WorkflowGta5LodSelectStep", "Execute",
                       "resident=" + std::to_string(state_->resident.size()),
                       "flagged " + std::to_string(changed) + " tiles");
    }
}

}  // namespace sdl3cpp::services::impl
