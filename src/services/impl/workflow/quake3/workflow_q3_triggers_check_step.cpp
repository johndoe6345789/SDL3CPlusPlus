#include "services/interfaces/workflow/quake3/workflow_q3_triggers_check_step.hpp"
#include "services/interfaces/workflow/quake3/q3_trigger_load.hpp"
#include "services/interfaces/workflow/quake3/q3_trigger_overlap.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowQ3TriggersCheckStep::WorkflowQ3TriggersCheckStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3TriggersCheckStep::GetPluginId() const {
    return "q3.triggers.check";
}

void WorkflowQ3TriggersCheckStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (context.GetBool("q3.player_dead", false)) return;

    LoadQ3TriggersIfNeeded(context, logger_);

    const auto* triggerListPtr =
        context.TryGet<nlohmann::json>("q3.trigger_list");
    if (!triggerListPtr || !triggerListPtr->is_array()) return;

    const auto* destIdxPtr =
        context.TryGet<nlohmann::json>("q3.trigger_dest_index");
    const glm::vec3 playerPos = ReadQ3TriggerPlayerPos(context);

    ApplyQ3TriggerOverlaps(context, *triggerListPtr, destIdxPtr, playerPos);
}

}  // namespace sdl3cpp::services::impl
