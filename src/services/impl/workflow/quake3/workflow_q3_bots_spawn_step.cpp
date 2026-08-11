#include "services/interfaces/workflow/quake3/workflow_q3_bots_spawn_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowQ3BotsSpawnStep::WorkflowQ3BotsSpawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3BotsSpawnStep::GetPluginId() const { return "q3.bots.spawn"; }

void WorkflowQ3BotsSpawnStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    // Idempotent: only spawn once
    const auto* existing = context.TryGet<nlohmann::json>("q3.bots");
    if (existing && existing->is_array() && !existing->empty()) return;

    WorkflowStepParameterResolver params;
    const auto* cp = params.FindParameter(step, "count");
    const int maxBots = (cp && cp->type == WorkflowParameterValue::Type::Number)
                      ? (int)cp->numberValue : 4;

    // Collect all deathmatch spawn points from BSP entities
    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
    std::vector<nlohmann::json> spawnPts;
    if (entities && entities->is_array()) {
        for (const auto& ent : *entities) {
            const std::string cls = ent.value("classname", std::string{});
            if ((cls == "info_player_deathmatch" || cls == "info_player_start")
                    && ent.contains("position")) {
                spawnPts.push_back(ent["position"]);
            }
        }
    }

    if (spawnPts.empty()) {
        // Fallback: scatter bots near origin
        for (int i = 0; i < maxBots; ++i) {
            const float a = (float)i * 1.57f;
            spawnPts.push_back(nlohmann::json::array(
                {std::cos(a) * 3.0f, 0.5f, std::sin(a) * 3.0f}));
        }
    }

    nlohmann::json bots = nlohmann::json::array();
    // Skip index 0 — that spawn point is reserved for the player (same one
    // bsp.parse_spawn selects).  If there are 2+ spawn points start bots from
    // index 1 so they never overlap the player.  Fall back to index 0 only
    // when the map has a single spawn point (no better option).
    const int startIdx = (spawnPts.size() > 1) ? 1 : 0;
    const int available = (int)spawnPts.size() - startIdx;
    const int count = std::min(maxBots, available > 0 ? available : maxBots);
    for (int i = 0; i < count; ++i) {
        const auto& sp = spawnPts[(size_t)(startIdx + i) % spawnPts.size()];
        nlohmann::json bot;
        bot["id"]          = i;
        bot["pos"]         = sp;
        bot["yaw"]         = 0.0f;
        bot["health"]      = 100;
        bot["state"]       = "idle";  // idle | chase | shoot | dead
        bot["leg_frame"]   = 0;
        bot["torso_frame"] = 0;
        bot["weapon"]      = "weapon_machinegun";
        bot["last_shot"]   = 0;
        bots.push_back(bot);
    }
    context.Set("q3.bots", bots);
    if (logger_) logger_->Info("q3.bots.spawn: spawned " + std::to_string(count) + " bots");
}

}  // namespace sdl3cpp::services::impl
