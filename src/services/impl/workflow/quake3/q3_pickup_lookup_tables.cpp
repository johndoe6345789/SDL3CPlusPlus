#include "services/interfaces/workflow/quake3/q3_pickup_lookup_tables.hpp"

namespace sdl3cpp::services::impl {

int DefaultAmmoAmount(const std::string& cls) {
    if (cls == "ammo_bullets") return 50;
    if (cls == "ammo_shells") return 10;
    if (cls == "ammo_grenades") return 5;
    if (cls == "ammo_rockets") return 5;
    if (cls == "ammo_cells") return 30;
    if (cls == "ammo_lightning") return 60;
    if (cls == "ammo_slugs") return 10;
    if (cls == "ammo_bfg") return 15;
    return 10;
}

std::string AmmoWeaponKey(const std::string& cls) {
    if (cls == "ammo_bullets") return "weapon_machinegun";
    if (cls == "ammo_shells") return "weapon_shotgun";
    if (cls == "ammo_grenades") return "weapon_grenadelauncher";
    if (cls == "ammo_rockets") return "weapon_rocketlauncher";
    if (cls == "ammo_cells") return "weapon_plasmagun";
    if (cls == "ammo_lightning") return "weapon_lightning";
    if (cls == "ammo_slugs") return "weapon_railgun";
    if (cls == "ammo_bfg") return "weapon_bfg";
    return cls;  // fallback: use classname as key
}

int DefaultWeaponAmmo(const std::string& cls) {
    if (cls == "weapon_machinegun") return 40;
    if (cls == "weapon_shotgun") return 10;
    if (cls == "weapon_grenadelauncher") return 10;
    if (cls == "weapon_rocketlauncher") return 10;
    if (cls == "weapon_lightning") return 100;
    if (cls == "weapon_railgun") return 10;
    if (cls == "weapon_plasmagun") return 50;
    if (cls == "weapon_bfg") return 20;
    return 10;
}

int HealthQuantity(const std::string& cls) {
    if (cls == "item_health_small") return 5;
    if (cls == "item_health_large") return 50;
    if (cls == "item_health_mega") return 100;
    return 25;  // item_health
}

int ArmorQuantity(const std::string& cls) {
    if (cls == "item_armor_shard") return 5;
    if (cls == "item_armor_body") return 100;
    return 50;  // item_armor_combat
}

}  // namespace sdl3cpp::services::impl
