#include "services/interfaces/workflow/quake3/workflow_q3_stats_init_step.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {
namespace {
// g_client.c: ent->health = ps.stats[STAT_HEALTH]
//                         = ps.stats[STAT_MAX_HEALTH] + 25
constexpr int kMaxHealth = 100;
constexpr int kSpawnHealth = kMaxHealth + 25;
constexpr int kSpawnArmor = 0;
}  // namespace

WorkflowQ3StatsInitStep::WorkflowQ3StatsInitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3StatsInitStep::GetPluginId() const {
    return "q3.stats.init";
}

void WorkflowQ3StatsInitStep::Execute(const WorkflowStepDefinition&,
                                      WorkflowContext& context) {
    if (context.Contains("q3.player_health")) {
        return;  // a respawn or pickup already owns these
    }

    context.Set<int>("q3.player_max_health", kMaxHealth);
    context.Set<int>("q3.player_health", kSpawnHealth);
    context.Set<int>("q3.player_armor", kSpawnArmor);

    if (logger_) {
        logger_->Info("q3.stats.init: health " +
                      std::to_string(kSpawnHealth) + ", armor " +
                      std::to_string(kSpawnArmor));
    }
}

}  // namespace sdl3cpp::services::impl
