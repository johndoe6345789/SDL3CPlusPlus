#include "services/interfaces/workflow/quake3/workflow_q3_ammo_init_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3AmmoInitStep::WorkflowQ3AmmoInitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3AmmoInitStep::GetPluginId() const { return "q3.ammo.init"; }

void WorkflowQ3AmmoInitStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    // One-shot guard
    if (context.GetBool("q3.ammo_initialized", false)) return;

    nlohmann::json ammo = {
        {"weapon_machinegun",     100},
        {"weapon_shotgun",         10},
        {"weapon_grenadelauncher",  5},
        {"weapon_rocketlauncher",   5},
        {"weapon_lightning",       60},
        {"weapon_railgun",         10},
        {"weapon_plasmagun",       50},
        {"weapon_bfg",             15}
    };

    context.Set("q3.player_ammo",      ammo);
    context.Set("q3.ammo_initialized", true);

    if (logger_) logger_->Info("q3.ammo.init: ammo initialized");
}

}  // namespace sdl3cpp::services::impl
