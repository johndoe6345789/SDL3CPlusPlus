#include "services/interfaces/workflow/quake3/workflow_q3_player_respawn_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <string>

namespace sdl3cpp::services::impl {

static constexpr double kRespawnDelay = 1.7;

WorkflowQ3PlayerRespawnStep::WorkflowQ3PlayerRespawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PlayerRespawnStep::GetPluginId() const { return "q3.player.respawn"; }

void WorkflowQ3PlayerRespawnStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (!context.GetBool("q3.player_dead", false)) return;

    const double elapsed   = context.GetDouble("frame.elapsed", 0.0);
    const double deathTime = context.GetDouble("q3.death_time", 0.0);
    if ((elapsed - deathTime) <= kRespawnDelay) return;

    // Pick a random spawn point
    glm::vec3 spawnPos(0.0f, 2.0f, 0.0f);
    const auto* spawnPoints = context.TryGet<nlohmann::json>("bsp.spawn_points");
    if (spawnPoints && spawnPoints->is_array() && !spawnPoints->empty()) {
        const int idx = std::rand() % static_cast<int>(spawnPoints->size());
        const auto& sp = (*spawnPoints)[idx];
        if (sp.is_array() && sp.size() == 3) {
            spawnPos = glm::vec3(sp[0].get<float>(), sp[1].get<float>(), sp[2].get<float>());
        } else if (sp.is_object()) {
            spawnPos = glm::vec3(
                sp.value("x", 0.0f),
                sp.value("y", 2.0f),
                sp.value("z", 0.0f));
        }
    }

    context.Set("q3.player_pos",      spawnPos);
    context.Set("q3.player_health",   100);
    context.Set("q3.player_armor",    0);
    context.Set("q3.armor_type",      std::string("none"));
    context.Set("q3.player_dead",     false);
    // Trigger ammo re-init on next frame
    context.Set("q3.ammo_initialized", false);

    if (logger_) {
        logger_->Info("q3.player.respawn: respawned at (" +
                      std::to_string(spawnPos.x) + ", " +
                      std::to_string(spawnPos.y) + ", " +
                      std::to_string(spawnPos.z) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
