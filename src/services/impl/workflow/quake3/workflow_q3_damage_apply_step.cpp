#include "services/interfaces/workflow/quake3/workflow_q3_damage_apply_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3DamageApplyStep::WorkflowQ3DamageApplyStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3DamageApplyStep::GetPluginId() const { return "q3.damage.apply"; }

void WorkflowQ3DamageApplyStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    const int damage = context.GetInt("q3.pending_damage", 0);
    if (damage == 0) return;

    int health    = context.GetInt("q3.player_health", 100);
    int armor     = context.GetInt("q3.player_armor",  0);
    const std::string armorType = context.GetString("q3.armor_type", "none");

    // ioq3 G_Damage armor absorption
    // yellow armor: protection = 1.0, green armor: protection = 0.667
    if (armor > 0 && armorType != "none") {
        const double protection = (armorType == "yellow") ? 1.0 : 0.667;
        int save = static_cast<int>(std::ceil(damage * protection));
        save     = std::min(save, armor);
        armor   -= save;
        health  -= (damage - save);
    } else {
        health -= damage;
    }

    context.Set("q3.player_health",  health);
    context.Set("q3.player_armor",   armor);
    context.Set("q3.pending_damage", 0);

    if (logger_) {
        logger_->Info("q3.damage.apply: damage=" + std::to_string(damage) +
                      " health=" + std::to_string(health) +
                      " armor=" + std::to_string(armor));
    }
}

}  // namespace sdl3cpp::services::impl
