#include "services/interfaces/workflow/quake3/workflow_q3_weapon_select_step.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

const std::array<const char*, 9> kWeapons = {
    "weapon_gauntlet",
    "weapon_machinegun",
    "weapon_shotgun",
    "weapon_grenadelauncher",
    "weapon_rocketlauncher",
    "weapon_lightning",
    "weapon_railgun",
    "weapon_plasmagun",
    "weapon_bfg"
};

bool HasWeapon(const nlohmann::json& inventory, const std::string& weapon) {
    return weapon == "weapon_gauntlet" ||
           weapon == "weapon_machinegun" ||
           inventory.value(weapon, false);
}

}  // namespace

WorkflowQ3WeaponSelectStep::WorkflowQ3WeaponSelectStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3WeaponSelectStep::GetPluginId() const {
    return "q3.weapon.select";
}

void WorkflowQ3WeaponSelectStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    auto inventory = context.Get<nlohmann::json>("q3.inventory", nlohmann::json::object());
    // Gauntlet and machinegun are always available.
    inventory["weapon_gauntlet"]  = true;
    inventory["weapon_machinegun"] = true;

    std::string current = context.Get<std::string>("q3.current_weapon", "weapon_machinegun");

    for (size_t i = 0; i < kWeapons.size(); ++i) {
        if (!context.GetBool("input_key_" + std::to_string(i + 1), false)) continue;
        const std::string requested = kWeapons[i];
        if (HasWeapon(inventory, requested)) {
            current = requested;
        }
    }

    context.Set("q3.inventory", inventory);
    context.Set<std::string>("q3.current_weapon", current);
}

}  // namespace sdl3cpp::services::impl
