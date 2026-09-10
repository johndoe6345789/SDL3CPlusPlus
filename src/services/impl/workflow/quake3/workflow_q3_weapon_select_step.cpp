#include "services/interfaces/workflow/quake3/workflow_q3_weapon_select_step.hpp"

#include "services/interfaces/workflow/quake3/q3_weapon_cycle.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

bool HasWeapon(const nlohmann::json& inventory, const std::string& weapon) {
    return weapon == "weapon_gauntlet" || weapon == "weapon_machinegun" ||
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

    const auto ammo =
        context.Get<nlohmann::json>("q3.player_ammo", nlohmann::json::object());
    std::string current = context.Get<std::string>("q3.current_weapon", "weapon_machinegun");

    // Selecting by number needs only the weapon, as Quake's `weapon N`
    // does; it does not check ammo the way cycling does.
    const auto& order = Q3WeaponOrder();
    for (size_t i = 0; i < order.size(); ++i) {
        if (!context.GetBool("input_key_" + std::to_string(i + 1), false)) continue;
        const std::string requested = order[i];
        if (HasWeapon(inventory, requested)) {
            current = requested;
        }
    }

    // The wheel arrives as notches for the frame: one weapon per notch,
    // up going forward, which is how Quake binds mwheelup/mwheeldown.
    const int notches = static_cast<int>(
        std::lround(context.Get<float>("input_mouse_wheel_y", 0.0f)));
    for (int n = 0; n < std::abs(notches); ++n) {
        current = CycleWeapon(inventory, ammo, current, notches > 0 ? 1 : -1);
    }

    context.Set("q3.inventory", inventory);
    context.Set<std::string>("q3.current_weapon", current);
    // What the viewmodel draw binds its prefix to.
    context.Set<std::string>("q3.weapon_prefix", Q3WeaponModelPrefix(current));
}

}  // namespace sdl3cpp::services::impl
