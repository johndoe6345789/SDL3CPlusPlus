#include "services/interfaces/workflow/quake3/workflow_q3_player_death_check_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3PlayerDeathCheckStep::WorkflowQ3PlayerDeathCheckStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PlayerDeathCheckStep::GetPluginId() const { return "q3.player.death_check"; }

void WorkflowQ3PlayerDeathCheckStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    const int  health  = context.GetInt("q3.player_health", 100);
    const bool isDead  = context.GetBool("q3.player_dead",  false);

    // Only trigger on the first frame health drops to/below zero
    if (health > 0 || isDead) return;

    const double elapsed    = context.GetDouble("frame.elapsed", 0.0);
    const int    deathCount = context.GetInt("q3.death_count", 0);

    context.Set("q3.player_dead",  true);
    context.Set("q3.death_time",   elapsed);
    context.Set("q3.death_count",  deathCount + 1);

    if (logger_) {
        logger_->Info("q3.player.death_check: player died (count=" +
                      std::to_string(deathCount + 1) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
