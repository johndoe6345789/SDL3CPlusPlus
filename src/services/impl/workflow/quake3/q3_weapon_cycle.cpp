#include "services/interfaces/workflow/quake3/q3_weapon_cycle.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kGauntlet = "weapon_gauntlet";

}  // namespace

const std::array<const char*, 9>& Q3WeaponOrder() {
    static const std::array<const char*, 9> kOrder = {
        "weapon_gauntlet",        "weapon_machinegun",
        "weapon_shotgun",         "weapon_grenadelauncher",
        "weapon_rocketlauncher",  "weapon_lightning",
        "weapon_railgun",         "weapon_plasmagun",
        "weapon_bfg"};
    return kOrder;
}

bool IsWeaponSelectable(const nlohmann::json& inventory,
                        const nlohmann::json& ammo,
                        const std::string& weapon) {
    if (!inventory.value(weapon, false)) {
        return false;
    }
    return ammo.value(weapon, 0) > 0;
}

std::string CycleWeapon(const nlohmann::json& inventory,
                        const nlohmann::json& ammo, const std::string& current,
                        int direction) {
    const auto& order = Q3WeaponOrder();
    const auto count  = static_cast<int>(order.size());

    int index = 0;
    for (int i = 0; i < count; ++i) {
        if (current == order[static_cast<size_t>(i)]) {
            index = i;
            break;
        }
    }

    const int step = direction >= 0 ? 1 : -1;
    for (int taken = 0; taken < count; ++taken) {
        index = (index + step + count) % count;
        const std::string candidate = order[static_cast<size_t>(index)];
        if (candidate == kGauntlet) {
            continue;  // Quake never cycles onto the gauntlet
        }
        if (IsWeaponSelectable(inventory, ammo, candidate)) {
            return candidate;
        }
    }
    return current;
}

std::string Q3WeaponModelPrefix(const std::string& weapon) {
    if (weapon == "weapon_gauntlet") return "weapon_gauntlet";
    if (weapon == "weapon_shotgun") return "weapon_shotgun";
    if (weapon == "weapon_grenadelauncher") return "weapon_grenadel";
    if (weapon == "weapon_rocketlauncher") return "weapon_rocketl";
    if (weapon == "weapon_lightning") return "weapon_lightning";
    if (weapon == "weapon_railgun") return "weapon_railgun";
    if (weapon == "weapon_plasmagun") return "weapon_plasma";
    if (weapon == "weapon_bfg") return "weapon_bfg";
    return "weapon_mg";  // machinegun, and the fallback
}

}  // namespace sdl3cpp::services::impl
