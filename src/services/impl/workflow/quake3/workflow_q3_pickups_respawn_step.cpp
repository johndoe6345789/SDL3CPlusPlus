#include "services/interfaces/workflow/quake3/workflow_q3_pickups_respawn_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowQ3PickupsRespawnStep::WorkflowQ3PickupsRespawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PickupsRespawnStep::GetPluginId() const { return "q3.pickups.respawn"; }

void WorkflowQ3PickupsRespawnStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    const auto* respawnTimesPtr = context.TryGet<nlohmann::json>("q3.pickup_respawn_times");
    if (!respawnTimesPtr || !respawnTimesPtr->is_object() || respawnTimesPtr->empty()) return;

    const double elapsed = context.GetDouble("frame.elapsed", 0.0);
    auto collected       = context.Get<nlohmann::json>("q3.collected", nlohmann::json::object());

    std::vector<std::string> toRespawn;
    for (auto it = respawnTimesPtr->begin(); it != respawnTimesPtr->end(); ++it) {
        const double respawnAt = it.value().get<double>();
        if (elapsed > respawnAt) {
            toRespawn.push_back(it.key());
        }
    }

    if (toRespawn.empty()) return;

    for (const auto& id : toRespawn) {
        collected[id] = false;
        if (logger_) logger_->Info("q3.pickups.respawn: respawned pickup id=" + id);
    }

    context.Set("q3.collected", collected);
}

}  // namespace sdl3cpp::services::impl
