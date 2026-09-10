#include "services/interfaces/workflow/quake3/q3_weapon_stats.hpp"

namespace sdl3cpp::q3 {

uint32_t WeaponFireInterval(const std::string& weapon) {
    if (weapon == "weapon_machinegun") return 6u;
    if (weapon == "weapon_shotgun") return 30u;
    if (weapon == "weapon_grenadelauncher") return 30u;
    if (weapon == "weapon_rocketlauncher") return 20u;
    if (weapon == "weapon_lightning") return 1u;
    if (weapon == "weapon_railgun") return 50u;
    if (weapon == "weapon_plasmagun") return 6u;
    if (weapon == "weapon_bfg") return 200u;
    return 18u;  // gauntlet fallback
}

bool IsWeaponAutoFire(const std::string& weapon) {
    return weapon == "weapon_machinegun" || weapon == "weapon_lightning" ||
           weapon == "weapon_plasmagun";
}

int WeaponInstantHitDamage(const std::string& weapon) {
    if (weapon == "weapon_machinegun") return 7;
    if (weapon == "weapon_shotgun") return 10;  // x11 pellets
    if (weapon == "weapon_lightning") return 8;
    if (weapon == "weapon_railgun") return 100;
    return 0;
}

}  // namespace sdl3cpp::q3
