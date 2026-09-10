#include "services/interfaces/workflow/quake3/workflow_q3_pickups_touch_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_effects.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3PickupsTouchStep::WorkflowQ3PickupsTouchStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PickupsTouchStep::GetPluginId() const {
    return "q3.pickups.touch";
}

void WorkflowQ3PickupsTouchStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (context.GetBool("q3.player_dead", false)) return;

    const auto* entitiesPtr = context.TryGet<nlohmann::json>("bsp.entities");
    if (!entitiesPtr || !entitiesPtr->is_array()) return;

    const glm::vec3 playerPos =
        context.Get<glm::vec3>("q3.player_pos", glm::vec3(0.0f));
    const double elapsed = context.GetDouble("frame.elapsed", 0.0);

    PickupTouchState state;
    state.health    = context.GetInt("q3.player_health", 100);
    state.armor     = context.GetInt("q3.player_armor", 0);
    state.armorType = context.GetString("q3.armor_type", "none");
    state.ammo = context.Get<nlohmann::json>("q3.player_ammo",
                                              nlohmann::json::object());
    state.inventory = context.Get<nlohmann::json>("q3.inventory",
                                                   nlohmann::json::object());
    state.collected = context.Get<nlohmann::json>("q3.collected",
                                                   nlohmann::json::object());
    state.respawnTimes = context.Get<nlohmann::json>(
        "q3.pickup_respawn_times", nlohmann::json::object());

    ApplyPickupTouches(*entitiesPtr, playerPos, elapsed, state, logger_);

    context.Set("q3.player_health", state.health);
    context.Set("q3.player_armor", state.armor);
    context.Set("q3.armor_type", state.armorType);
    context.Set("q3.player_ammo", state.ammo);
    context.Set("q3.inventory", state.inventory);
    context.Set("q3.collected", state.collected);
    context.Set("q3.pickup_respawn_times", state.respawnTimes);
}

}  // namespace sdl3cpp::services::impl
