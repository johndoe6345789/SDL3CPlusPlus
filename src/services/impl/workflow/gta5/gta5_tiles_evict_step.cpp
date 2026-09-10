#include "services/interfaces/workflow/gta5/gta5_tiles_evict_step.hpp"

#include "services/interfaces/scene_types.hpp"
#include "services/interfaces/workflow/gta5/gta5_evict_plan.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGta5TilesEvictStep::WorkflowGta5TilesEvictStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5TilesEvictStep::GetPluginId() const {
    return "gta5.tiles.evict";
}

void WorkflowGta5TilesEvictStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!state_ || state_->resident.empty()) return;

    const std::string objectsKey =
        Gta5ParameterOr(step, "objects_key", "scene_objects");
    const Gta5EvictPlan plan = ApplyGta5EvictPlan(
        *state_, Gta5ParameterOr(step, "object_type", ""));
    if (plan.purge.empty()) return;

    const auto* existing =
        context.TryGet<std::vector<SceneObject>>(objectsKey);
    if (!existing) return;

    std::vector<SceneObject> remaining;
    remaining.reserve(existing->size());
    for (const auto& object : *existing) {
        if (plan.purge.find(object.objectType) == plan.purge.end()) {
            remaining.push_back(object);
        }
    }

    const std::size_t removed = existing->size() - remaining.size();
    context.Set(objectsKey, std::move(remaining));

    if (logger_) {
        logger_->Trace("WorkflowGta5TilesEvictStep", "Execute",
                       "dropped=" + std::to_string(plan.dropped.size()) +
                           " rebuilt=" + std::to_string(plan.rebuilt),
                       "removed " + std::to_string(removed) + " objects");
    }
}

}  // namespace sdl3cpp::services::impl
